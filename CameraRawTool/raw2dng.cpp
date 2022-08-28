#include <iostream>
#include <assert.h>

#include "dng_color_space.h"
#include "dng_date_time.h"
#include "dng_exceptions.h"
#include "dng_file_stream.h"
#include "dng_globals.h"
#include "dng_host.h"
#include "dng_ifd.h"
#include "dng_image_writer.h"
#include "dng_info.h"
#include "dng_linearization_info.h"
#include "dng_mosaic_info.h"
#include "dng_negative.h"
#include "dng_preview.h"
#include "dng_render.h"
#include "dng_simple_image.h"
#include "dng_tag_codes.h"
#include "dng_tag_types.h"
#include "dng_tag_values.h"
#include "dng_xmp.h"
#include "dng_xmp_sdk.h"
#include "dng_camera_profile.h"

#include "cJSON/cJSON.h"
// --------------------------------------------------------------------------------
//
// MakeDNGSample
//

struct MetaData
{
	// reserve information (rarely changed)
	uint8 color_planes;

	// basic information
	std::string input_file;
	std::string camera_maker;
	std::string camera_model;
	std::string profile_name;
	std::string profile_copy_right;
	uint32 width;
	uint32 height;
	std::vector<uint32> ActiveArea;
	std::vector<uint32> DefaultCropOrigin;
	std::vector<uint32> DefaultCropSize;
	uint32 CfaLayout;
	uint32 bit_depth;
	std::vector<uint32> BlackLevel;
	uint32 WhiteLevel;
	float rgain;
	float bgain;

	float AntiAliasStrength;

	uint32 ISO;
	double exposure_time;
	double lens_aperture;
	double focal_length;

	// output information
	std::string output_file;
};

int load_meta_data(std::string& json_file, MetaData& meta_data)
{
	if (!json_file.empty())
	{
		FILE *fp = fopen(json_file.c_str(), "r");
		if (fp != NULL)
		{
			fseek(fp, 0, SEEK_END);
			uint64 length = ftell(fp);
			char *buffer = (char*)malloc(length + 1);
			//char buffer[65536];
			rewind(fp);
			fread(buffer, sizeof(char), length, fp);
			buffer[length] = '\0';

			cJSON *root = cJSON_Parse(buffer);
			//char* test = cJSON_Print(root);
			//LOG(DEBUG, "%s", test);
			if (root != nullptr)
			{
				cJSON *subitem = root->child;
				while (subitem)
				{
					if (!strcmp(subitem->string, "basic_information"))
					{
						cJSON *sub_basic = subitem->child;
						while (sub_basic)
						{
							if (!strcmp(sub_basic->string, "input_file"))
							{
								meta_data.input_file = std::string(sub_basic->valuestring);
							}
							else if (!strcmp(sub_basic->string, "camera_maker"))
							{
								meta_data.camera_maker = std::string(sub_basic->valuestring);
								meta_data.profile_copy_right = meta_data.camera_maker;
							}
							else if (!strcmp(sub_basic->string, "camera_model"))
							{
								meta_data.camera_model = std::string(sub_basic->valuestring);
								meta_data.profile_name = meta_data.camera_model;
							}
							else if (!strcmp(sub_basic->string, "width"))
							{
								meta_data.width = sub_basic->valueint;
							}
							else if (!strcmp(sub_basic->string, "height"))
							{
								meta_data.height = sub_basic->valueint;
							}
							else if (!strcmp(sub_basic->string, "ActiveArea"))
							{
								uint32 length = cJSON_GetArraySize(sub_basic);
								assert(length == 4);
								meta_data.ActiveArea.reserve(4);
								for (uint32 i = 0; i < length; ++i)
								{
									meta_data.ActiveArea.push_back(static_cast<uint32>(cJSON_GetArrayItem(sub_basic, i)->valueint));
								}
							}
							else if (!strcmp(sub_basic->string, "DefaultCropOrigin"))
							{
								uint32 length = cJSON_GetArraySize(sub_basic);
								assert(length == 2);
								meta_data.DefaultCropOrigin.reserve(2);
								for (uint32 i = 0; i < length; ++i)
								{
									meta_data.DefaultCropOrigin.push_back(static_cast<uint32>(cJSON_GetArrayItem(sub_basic, i)->valueint));
								}
							}
							else if (!strcmp(sub_basic->string, "DefaultCropSize"))
							{
								uint32 length = cJSON_GetArraySize(sub_basic);
								assert(length == 2);
								meta_data.DefaultCropSize.reserve(2);
								for (uint32 i = 0; i < length; ++i)
								{
									meta_data.DefaultCropSize.push_back(static_cast<uint32>(cJSON_GetArrayItem(sub_basic, i)->valueint));
								}
							}
							else if (!strcmp(sub_basic->string, "CfaLayout"))
							{
								meta_data.CfaLayout = sub_basic->valueint;
							}
							else if (!strcmp(sub_basic->string, "bit_depth"))
							{
								meta_data.bit_depth = sub_basic->valueint;
							}
							else if (!strcmp(sub_basic->string, "BlackLevel"))
							{
								uint32 length = cJSON_GetArraySize(sub_basic);
								meta_data.BlackLevel.reserve(4);
								meta_data.BlackLevel.push_back(static_cast<uint32>(cJSON_GetArrayItem(sub_basic, 0)->valueint));
								if (length == 4)
								{
									meta_data.BlackLevel.push_back(static_cast<uint32>(cJSON_GetArrayItem(sub_basic, 1)->valueint));
									meta_data.BlackLevel.push_back(static_cast<uint32>(cJSON_GetArrayItem(sub_basic, 2)->valueint));
									meta_data.BlackLevel.push_back(static_cast<uint32>(cJSON_GetArrayItem(sub_basic, 3)->valueint));
								}
								else
								{
									meta_data.BlackLevel.push_back(meta_data.BlackLevel[0]);
									meta_data.BlackLevel.push_back(meta_data.BlackLevel[0]);
									meta_data.BlackLevel.push_back(meta_data.BlackLevel[0]);
								}
							}
							else if (!strcmp(sub_basic->string, "WhiteLevel"))
							{
								meta_data.WhiteLevel = sub_basic->valueint;
							}
							else if (!strcmp(sub_basic->string, "rgain"))
							{
								meta_data.rgain = sub_basic->valuedouble;
							}
							else if (!strcmp(sub_basic->string, "bgain"))
							{
								meta_data.bgain = sub_basic->valuedouble;
							}
							else if (!strcmp(sub_basic->string, "AntiAliasStrength"))
							{
								meta_data.AntiAliasStrength = sub_basic->valuedouble;
							}
							else if (!strcmp(sub_basic->string, "ISO"))
							{
								meta_data.ISO = sub_basic->valueint;
							}
							else if (!strcmp(sub_basic->string, "exposure_time"))
							{
								meta_data.exposure_time = sub_basic->valuedouble;
							}
							else if (!strcmp(sub_basic->string, "lens_aperture"))
							{
								meta_data.lens_aperture = sub_basic->valuedouble;
							}
							else if (!strcmp(sub_basic->string, "focal_length"))
							{
								meta_data.focal_length = sub_basic->valuedouble;
							}
							else
							{
								if (sub_basic->string[0] != '#')
								{
									std::cout << "unspecified sub_basic flag = " << sub_basic->string << std::endl;
								}
							}
							sub_basic = sub_basic->next;
						}
					}
					else
					{
						if (subitem->string[0] != '#')
						{
							std::cout << "unspecified subitem flag = " << subitem->string << std::endl;
						}
					}
					subitem = subitem->next;
				}
				cJSON_Delete(root);
			}

			free(buffer);
			fclose(fp);
		}
		else
		{
			std::cout << "Unable to open file: " << json_file.c_str() << std::endl;
			return -1;
		}
	}

	return 0;
}

int set_reserve_meta_data(MetaData& meta_data)
{
	meta_data.color_planes = 1;

	return 0;
}

int raw2dng(std::string& raw_file, std::string& json_file, std::string &dcp_file)
{
	MetaData meta_data;
	load_meta_data(json_file, meta_data);
	set_reserve_meta_data(meta_data);

	// Create DNG
	try
	{
		dng_host oDNGHost;

		std::string base_file_name;
		size_t unIndex = raw_file.find_last_of(".");
		if (unIndex == std::string::npos)
		{
			base_file_name = raw_file;
		}
		else
		{
			base_file_name = raw_file.substr(0, unIndex);
		}
		std::string output_file = base_file_name + ".dng";

		// -------------------------------------------------------------
		// Print settings
		// -------------------------------------------------------------
		std::cout << "Converting: " << raw_file.c_str() << " to " << std::endl;
		std::cout << output_file.c_str() << std::endl;

		// -------------------------------------------------------------
		// BAYER input file settings
		// -------------------------------------------------------------
		dng_file_stream oBayerStream(raw_file.c_str());
		//oBayerStream.SetLittleEndian(false);

		uint32 ullBayerSize = (uint32)oBayerStream.Length();

		AutoPtr<dng_memory_block> oBayerData(oDNGHost.Allocate(ullBayerSize));

		oBayerStream.SetReadPosition(0);

		oBayerStream.Get(oBayerData->Buffer(), ullBayerSize);

		// -------------------------------------------------------------
		// DNG Host Settings
		// -------------------------------------------------------------

		// Set DNG version
		// Remarks: Tag [DNGVersion] / [50706]
		oDNGHost.SetSaveDNGVersion(dngVersion_SaveDefault);

		// Set DNG type to RAW DNG
		// Remarks: Store Bayer CFA data and not already processed data
		oDNGHost.SetSaveLinearDNG(false);

		// -------------------------------------------------------------
		// DNG Image Settings
		// -------------------------------------------------------------

		dng_rect vImageBounds(meta_data.height, meta_data.width);

		AutoPtr<dng_image> oImage(oDNGHost.Make_dng_image(vImageBounds, meta_data.color_planes, ttShort));

		dng_pixel_buffer oBuffer;

		oBuffer.fArea = vImageBounds;
		oBuffer.fPlane = 0;
		oBuffer.fPlanes = 1;
		oBuffer.fRowStep = oBuffer.fPlanes * meta_data.width;
		oBuffer.fColStep = oBuffer.fPlanes;
		oBuffer.fPlaneStep = 1;
		oBuffer.fPixelType = ttShort;
		oBuffer.fPixelSize = TagTypeSize(ttShort);
		oBuffer.fData = oBayerData->Buffer();

		oImage->Put(oBuffer);

		// -------------------------------------------------------------
		// DNG Negative Settings
		// -------------------------------------------------------------

		AutoPtr<dng_negative> oNegative(oDNGHost.Make_dng_negative());

		// Set camera model
		// Remarks: Tag [UniqueCameraModel] / [50708]
		oNegative->SetModelName(meta_data.camera_model.c_str());

		// Set localized camera model
		// Remarks: Tag [UniqueCameraModel] / [50709]
		oNegative->SetLocalName(meta_data.camera_model.c_str());

		// Set bayer pattern information
		// Remarks: Tag [CFAPlaneColor] / [50710] and [CFALayout] / [50711]
		oNegative->SetColorKeys(colorKeyRed, colorKeyGreen, colorKeyBlue);

		// Set bayer pattern information
		// Remarks: Tag [CFAPlaneColor] / [50710] and [CFALayout] / [50711]
		oNegative->SetBayerMosaic(meta_data.CfaLayout);

		// Set bayer pattern information
		// Remarks: Tag [CFAPlaneColor] / [50710] and [CFALayout] / [50711]
		oNegative->SetColorChannels(3);
		// Set linearization table
		// Remarks: Tag [LinearizationTable] / [50712]
		uint16 bit_limit = (0x01 << meta_data.bit_depth) - 1;
		AutoPtr<dng_memory_block> oCurve(oDNGHost.Allocate(sizeof(uint16) * bit_limit));
		for (int32 i = 0; i < bit_limit; i++)
		{
			uint16 *pulItem = oCurve->Buffer_uint16() + i;
			*pulItem = (uint16)(i);
		}
		oNegative->SetLinearization(oCurve);

		// Set black level to auto black level of sensor
		// Remarks: Tag [BlackLevel] / [50714]
		//oNegative->SetBlackLevel(meta_data.BlackLevel);
		oNegative->SetQuadBlacks(meta_data.BlackLevel[0], meta_data.BlackLevel[1], meta_data.BlackLevel[2], meta_data.BlackLevel[3]);

		// Set white level
		// Remarks: Tag [WhiteLevel] / [50717]
		oNegative->SetWhiteLevel(meta_data.WhiteLevel);

		// Set scale to square pixel
		// Remarks: Tag [DefaultScale] / [50718]
		oNegative->SetDefaultScale(dng_urational(1, 1), dng_urational(1, 1));

		// Set scale to square pixel
		// Remarks: Tag [BestQualityScale] / [50780]
		oNegative->SetBestQualityScale(dng_urational(1, 1));

		dng_rect activeArea(meta_data.ActiveArea[0], meta_data.ActiveArea[1], meta_data.ActiveArea[2], meta_data.ActiveArea[3]);
		oNegative->SetActiveArea(activeArea);

		// Set pixel area
		// Remarks: Tag [DefaultCropOrigin] / [50719]
		oNegative->SetDefaultCropOrigin(meta_data.DefaultCropOrigin[0], meta_data.DefaultCropOrigin[1]);

		// Set pixel area
		// Remarks: Tag [DefaultCropSize] / [50720]
		oNegative->SetDefaultCropSize(meta_data.DefaultCropSize[0], meta_data.DefaultCropSize[1]);

		// Set base orientation
		// Remarks: See Restriction / Extension tags chapter
		oNegative->SetBaseOrientation(dng_orientation::Normal());

		dng_vector as_shot_neutral(3);
		as_shot_neutral[0] = 1.0/meta_data.rgain;
		as_shot_neutral[1] = 1.0;
		as_shot_neutral[2] = 1.0/meta_data.bgain;
		//as_shot_neutral[3] = 1.0/1024;
		oNegative->SetCameraNeutral(as_shot_neutral);

		// Set camera white XY coordinates
		// Remarks: Tag [AsShotWhiteXY] / [50729]
		oNegative->SetCameraWhiteXY(D65_xy_coord());

		// Set baseline exposure
		// Remarks: Tag [BaselineExposure] / [50730]
		oNegative->SetBaselineExposure(0);

		uint32 denom = 1000000;
		dng_urational antiAliasStrength(uint32(meta_data.AntiAliasStrength*denom + 0.5), denom);
		oNegative->SetAntiAliasStrength(antiAliasStrength);

		// Set if noise reduction is already applied on RAW data
		// Remarks: Tag [NoiseReductionApplied] / [50935]
		oNegative->SetNoiseReductionApplied(dng_urational(0, 1));

		// Set baseline sharpness
		// Remarks: Tag [BaselineSharpness] / [50732]
		oNegative->SetBaselineSharpness(1);

		//oNegative->SetCameraCalibrationSignature("com.adobe");

		// -------------------------------------------------------------
		// DNG EXIF Settings
		// -------------------------------------------------------------

		dng_exif *poExif = oNegative->GetExif();

		// Set Camera Make
		// Remarks: Tag [Make] / [EXIF]
		poExif->fMake.Set_ASCII(meta_data.camera_maker.c_str());

		// Set Camera Model
		// Remarks: Tag [Model] / [EXIF]
		poExif->fModel.Set_ASCII(meta_data.camera_model.c_str());

		// Set ISO speed
		// Remarks: Tag [ISOSpeed] / [EXIF]
		poExif->fISOSpeedRatings[0] = meta_data.ISO;
		poExif->fISOSpeedRatings[1] = 0;
		poExif->fISOSpeedRatings[2] = 0;

		// Set WB mode
		// Remarks: Tag [WhiteBalance] / [EXIF]
		poExif->fWhiteBalance = 0;

		// Set metering mode
		// Remarks: Tag [MeteringMode] / [EXIF]
		poExif->fMeteringMode = 2;

		// Set metering mode
		// Remarks: Tag [ExposureBiasValue] / [EXIF]
		poExif->fExposureBiasValue = dng_srational(0, 0);

		// Set aperture value
		// Remarks: Tag [ApertureValue] / [EXIF]
		poExif->SetFNumber(meta_data.lens_aperture);

		// Set exposure time
		// Remarks: Tag [ExposureTime] / [EXIF]
		poExif->SetExposureTime(meta_data.exposure_time);

		// Set focal length
		// Remarks: Tag [FocalLength] / [EXIF]
		poExif->fFocalLength.Set_real64(meta_data.focal_length, 1000);

		// Set lens info
		// Remarks: Tag [LensInfo] / [EXIF]
		poExif->fLensInfo[0].Set_real64(meta_data.focal_length, 10);
		poExif->fLensInfo[1].Set_real64(meta_data.focal_length, 10);
		poExif->fLensInfo[2].Set_real64(meta_data.lens_aperture, 10);
		poExif->fLensInfo[3].Set_real64(meta_data.lens_aperture, 10);

		dng_file_stream profileStream(dcp_file.c_str());
		AutoPtr<dng_camera_profile> oProfile(new dng_camera_profile);
		oProfile->ParseExtended(profileStream);

		// Add camera profile to negative
		oNegative->AddProfile(oProfile);

		// -------------------------------------------------------------
		// Write DNG file
		// -------------------------------------------------------------

		// Assign Raw image data.
		oNegative->SetStage1Image(oImage);

		// Compute linearized and range mapped image
		oNegative->BuildStage2Image(oDNGHost);

		// Compute demosaiced image (used by preview and thumbnail)
		oNegative->BuildStage3Image(oDNGHost);

		// Update XMP / EXIF
		oNegative->SynchronizeMetadata();

		// Update IPTC
		oNegative->RebuildIPTC(true);

		// Create stream writer for output file
		dng_file_stream oDNGStream(output_file.c_str(), true);
		//oDNGStream.SetSwapBytes(1);

		// Write DNG file to disk
		AutoPtr<dng_image_writer> oWriter(new dng_image_writer());
		oWriter->WriteDNG(oDNGHost, oDNGStream, *oNegative.Get(), NULL, dngVersion_Current, ccUncompressed);
	}

	catch (const dng_exception &except)
	{

		return except.ErrorCode();

	}

	catch (...)
	{

		return dng_error_unknown;

	}

	std::cout << "Conversion complete" << std::endl;

	return 0;
}

int main(int argc, char *argv[])
{
	if (argc < 3)
	{
		std::cout << "Usage:  raw2dng.exe [raw file] [config json file] [option: custom dcp file]" << std::endl;
		std::cout << "If custom dcp file not set, raw2dng will use default dcp: Canon_EOS_550D.dcp" << std::endl;
	}
	else
	{
		std::string raw_file = std::string(argv[1]);
		std::string json_file = std::string(argv[2]);
		std::string dcp_file = "Canon_EOS_550D.dcp";
		if (argc > 3)
		{
			dcp_file = std::string(argv[3]);
		}
		raw2dng(raw_file, json_file, dcp_file);
	}

	return 0;
}
