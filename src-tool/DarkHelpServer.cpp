/* DarkHelp - C++ helper class for Darknet's C API.
 * Copyright 2019-2024 Stephane Charette <stephanecharette@gmail.com>
 * MIT license applies.  See "license.txt" for details.
 */

#include "DarkHelp.hpp"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <thread>

#include "json.hpp"

#ifdef WIN32
#pragma warning(disable: 4244)
#endif

size_t total_number_of_images_processed	= 0;
bool crop_and_save_detected_objects		= false;
bool save_annotated_image				= false;
bool save_txt_annotations				= false;
bool save_json_results					= false;
bool apply_roi							= false;
auto last_activity						= std::chrono::high_resolution_clock::now();
std::vector<cv::Rect> roi_rectangles;
std::filesystem::path roi_fn;
std::vector<std::string> messages;


nlohmann::json create_darkhelp_defaults()
{
	nlohmann::json j;

	j["darkhelp"]["lib"]["network"]["cfg"											] = "example.cfg";
	j["darkhelp"]["lib"]["network"]["names"											] = "example.names";
	j["darkhelp"]["lib"]["network"]["weights"										] = "example_best.weights";
	j["darkhelp"]["lib"]["network"]["bundle"										] = "";
	j["darkhelp"]["lib"]["network"]["bundle_key"									] = "";

	j["darkhelp"]["lib"]["settings"]["general"]["debug"								] = false;
	j["darkhelp"]["lib"]["settings"]["general"]["driver"							] = "darknet";
	j["darkhelp"]["lib"]["settings"]["general"]["threshold"							] = 0.5;
	j["darkhelp"]["lib"]["settings"]["general"]["non_maximal_suppression_threshold"	] = 0.45,
	j["darkhelp"]["lib"]["settings"]["general"]["modify_batch_and_subdivisions"		] = true;
	j["darkhelp"]["lib"]["settings"]["general"]["names_include_percentage"			] = true;
	j["darkhelp"]["lib"]["settings"]["general"]["fix_out_of_bound_values"			] = true;
	j["darkhelp"]["lib"]["settings"]["general"]["sort_predictions"					] = 0;

	j["darkhelp"]["lib"]["settings"]["annotation"]["auto_hide_labels"				] = true;
	j["darkhelp"]["lib"]["settings"]["annotation"]["shade_predictions"				] = 0.25;
	j["darkhelp"]["lib"]["settings"]["annotation"]["include_all_names"				] = true;
	j["darkhelp"]["lib"]["settings"]["annotation"]["font_scale"						] = 0.5;
	j["darkhelp"]["lib"]["settings"]["annotation"]["font_thickness"					] = 1;
	j["darkhelp"]["lib"]["settings"]["annotation"]["include_duration"				] = true;
	j["darkhelp"]["lib"]["settings"]["annotation"]["include_timestamp"				] = false;

	j["darkhelp"]["lib"]["settings"]["tiling"]["enable_tiles"						] = false;
	j["darkhelp"]["lib"]["settings"]["tiling"]["combine_tile_predictions"			] = true;
	j["darkhelp"]["lib"]["settings"]["tiling"]["only_combine_similar_predictions"	] = true;
	j["darkhelp"]["lib"]["settings"]["tiling"]["tile_edge_factor"					] = 0.25;
	j["darkhelp"]["lib"]["settings"]["tiling"]["tile_rect_factor"					] = 1.2;

	auto tmp = std::filesystem::temp_directory_path();
	auto input_dir	= tmp / "darkhelpserver" / "input";
	auto output_dir	= tmp / "darkhelpserver" / "output";

	j["darkhelp"]["server"]["settings"]["input_directory"							] = input_dir.string();
	j["darkhelp"]["server"]["settings"]["output_directory"							] = output_dir.string();
	j["darkhelp"]["server"]["settings"]["clear_output_directory_on_startup"			] = true;
	j["darkhelp"]["server"]["settings"]["save_annotated_image"						] = false;
	j["darkhelp"]["server"]["settings"]["save_txt_annotations"						] = false;
	j["darkhelp"]["server"]["settings"]["save_json_results"							] = true;
	j["darkhelp"]["server"]["settings"]["crop_and_save_detected_objects"			] = false;
	j["darkhelp"]["server"]["settings"]["exit_if_idle"								] = false;
	j["darkhelp"]["server"]["settings"]["idle_time_in_seconds"						] = 60;
	j["darkhelp"]["server"]["settings"]["max_images_to_process_at_once"				] = 1;
	j["darkhelp"]["server"]["settings"]["run_cmd_after_processing_images"			] = "";
	j["darkhelp"]["server"]["settings"]["purge_files_after_cmd_completes"			] = true;
	j["darkhelp"]["server"]["settings"]["use_camera_for_input"						] = false;

	j["darkhelp"]["server"]["settings"]["camera"]["save_original_image"				] = true;
	j["darkhelp"]["server"]["settings"]["camera"]["name"							] = "/dev/video0";
	j["darkhelp"]["server"]["settings"]["camera"]["width"							] = 640;
	j["darkhelp"]["server"]["settings"]["camera"]["height"							] = 480;
	j["darkhelp"]["server"]["settings"]["camera"]["fps"								] = 30;
	j["darkhelp"]["server"]["settings"]["camera"]["buffersize"						] = 2;

	j["darkhelp"]["server"]["settings"]["apply_roi"									] = false;

	return j;
}


void configure(DarkHelp::NN & nn, const nlohmann::json & j)
{
	// this one needs to be set prior to loading the network
	nn.config.modify_batch_and_subdivisions = j["darkhelp"]["lib"]["settings"]["general"]["modify_batch_and_subdivisions"];

	DarkHelp::EDriver driver = DarkHelp::EDriver::kDarknet;
	const std::string driver_name = j["darkhelp"]["lib"]["settings"]["general"]["driver"];
	if (driver_name == "opencv")
	{
		driver = DarkHelp::EDriver::kOpenCV;
	}
	else if (driver_name == "opencvcpu")
	{
		driver = DarkHelp::EDriver::kOpenCVCPU;
	}
	else if (driver_name != "darknet")
	{
		throw std::invalid_argument("driver name \"" + driver_name + "\" is invalid");
	}

	if (j["darkhelp"]["lib"]["network"]["bundle"].empty())
	{
		nn.init(
			j["darkhelp"]["lib"]["network"]["cfg"		],
			j["darkhelp"]["lib"]["network"]["weights"	],
			j["darkhelp"]["lib"]["network"]["names"		],
			true,
			driver);
	}
	else
	{
		nn.init(false, j["darkhelp"]["lib"]["network"]["bundle"], j["darkhelp"]["lib"]["network"]["bundle_key"], driver);
	}

	if (j["darkhelp"]["lib"]["settings"]["general"]["debug"])
	{
		nn.config.enable_debug = true;
	}

	std::cout
		<< "-> using DarkHelp v"		<< DarkHelp::version()	<< std::endl
		<< "-> using driver \""			<< driver_name << "\""	<< std::endl
		<< "-> network loaded in "		<< nn.duration_string()	<< std::endl
		<< "-> network dimensions: "	<< nn.network_size()	<< std::endl
		<< "-> number of classes: "		<< nn.names.size()		<< std::endl;

	for (size_t idx = 0; idx < nn.names.size(); idx ++)
	{
		std::cout << "   " << idx << " = " << nn.names.at(idx) << std::endl;
	}

	nn.config.threshold							= j["darkhelp"]["lib"]["settings"]["general"]["threshold"];
	nn.config.non_maximal_suppression_threshold	= j["darkhelp"]["lib"]["settings"]["general"]["non_maximal_suppression_threshold"	];
	nn.config.names_include_percentage			= j["darkhelp"]["lib"]["settings"]["general"]["names_include_percentage"			];
	nn.config.fix_out_of_bound_values			= j["darkhelp"]["lib"]["settings"]["general"]["fix_out_of_bound_values"				];
	nn.config.sort_predictions					= j["darkhelp"]["lib"]["settings"]["general"]["sort_predictions"					];
	nn.config.annotation_auto_hide_labels		= j["darkhelp"]["lib"]["settings"]["annotation"]["auto_hide_labels"					];
	nn.config.annotation_shade_predictions		= j["darkhelp"]["lib"]["settings"]["annotation"]["shade_predictions"				];
	nn.config.include_all_names					= j["darkhelp"]["lib"]["settings"]["annotation"]["include_all_names"				];
	nn.config.annotation_font_face				= j["darkhelp"]["lib"]["settings"]["annotation"]["font_scale"						];
	nn.config.annotation_font_thickness			= j["darkhelp"]["lib"]["settings"]["annotation"]["font_thickness"					];
	nn.config.annotation_include_duration		= j["darkhelp"]["lib"]["settings"]["annotation"]["include_duration"					];
	nn.config.annotation_include_timestamp		= j["darkhelp"]["lib"]["settings"]["annotation"]["include_timestamp"				];
	nn.config.enable_tiles						= j["darkhelp"]["lib"]["settings"]["tiling"]["enable_tiles"							];
	nn.config.combine_tile_predictions			= j["darkhelp"]["lib"]["settings"]["tiling"]["combine_tile_predictions"				];
	nn.config.only_combine_similar_predictions	= j["darkhelp"]["lib"]["settings"]["tiling"]["only_combine_similar_predictions"		];
	nn.config.tile_edge_factor					= j["darkhelp"]["lib"]["settings"]["tiling"]["tile_edge_factor"						];
	nn.config.tile_rect_factor					= j["darkhelp"]["lib"]["settings"]["tiling"]["tile_rect_factor"						];

	return;
}


nlohmann::json merge(const nlohmann::json & lhs, const nlohmann::json & rhs)
{
	nlohmann::json j = lhs;

	// if a key exists in both LHS and RHS, then we keep RHS

	for (auto it = rhs.cbegin(); it != rhs.cend(); ++it)
	{
		const auto & key = it.key();
		if (it->is_object())
		{
			if (lhs.contains(key))
			{
				j[key] = merge(lhs[key], rhs[key]);
			}
			else
			{
				messages.push_back("WARNING: The object \"" + key + "\" seems to be unknown: " + rhs.dump());
				j[key] = rhs[key];
			}
		}
		else
		{
			if (j.contains(key) == false)
			{
				messages.push_back("WARNING: The key \"" + key + "\" seems to be unknown: " + rhs.dump());
			}
			j[key] = it.value();
		}
	}

	return j;
}


void load_roi(const std::filesystem::path & src)
{
	if (apply_roi == false)
	{
		return;
	}

	roi_rectangles.clear();

	roi_fn = std::filesystem::path(src).replace_extension(".roi");
	if (std::filesystem::exists(roi_fn) == false)
	{
		roi_fn.clear();
		return;
	}

	std::ifstream ifs(roi_fn);
	while (ifs.good())
	{
		int x = -1;
		int y = -1;
		int w = -1;
		int h = -1;
		ifs >> x >> y >> w >> h;

		if (x >= 0 and y >= 0 and w > 0 and h > 0)
		{
			roi_rectangles.emplace_back(cv::Rect(x, y, w, h));
		}
	}

	std::cout << "Number of RoI defined in " << roi_fn << ": " << roi_rectangles.size() << std::endl;
	for (const auto & r : roi_rectangles)
	{
		std::cout << "-> " << r << std::endl;
	}

	return;
}


void process_image(DarkHelp::NN & nn, cv::Mat & mat, const std::string & stem)
{
	if (mat.empty())
	{
		return;
	}

	const auto now = std::chrono::high_resolution_clock::now();

	total_number_of_images_processed ++;
	last_activity = now;

	const auto results = nn.predict(mat);

	std::string annotated_filename;
	if (save_annotated_image)
	{
		annotated_filename = stem + "_annotated.jpg";
		auto annotated_image = nn.annotate();
		for (const auto & r : roi_rectangles)
		{
			cv::rectangle(annotated_image, r, cv::Scalar(0, 255, 0));
			cv::rectangle(annotated_image, cv::Point(r.x - 1, r.y - 1), cv::Point(r.x + r.width + 1, r.y + r.height + 1), cv::Scalar(0, 0, 255));
		}
		cv::imwrite(annotated_filename, annotated_image, {cv::ImwriteFlags::IMWRITE_JPEG_QUALITY, 70});
	}

	std::string txt_filename;
	if (save_txt_annotations)
	{
		txt_filename = stem + ".txt";
		std::ofstream ofs(txt_filename);
		ofs << std::fixed << std::setprecision(10);
		for (const auto & prediction : results)
		{
			ofs	<< prediction.best_class			<< " "
				<< prediction.original_point.x		<< " "
				<< prediction.original_point.y		<< " "
				<< prediction.original_size.width	<< " "
				<< prediction.original_size.height	<< std::endl;
		}
	}

	if (save_json_results)
	{
		nlohmann::json output;

		const auto epoch			= now.time_since_epoch();
		const auto nanoseconds		= std::chrono::duration_cast<std::chrono::nanoseconds>	(epoch).count();
		const std::time_t seconds	= std::chrono::duration_cast<std::chrono::seconds>		(epoch).count();
		const auto lt				= std::localtime(&seconds);
		char buffer[50];
		std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S %z", lt);

		output["timestamp"]["nanoseconds"	] = nanoseconds;
		output["timestamp"]["epoch"			] = seconds;
		output["timestamp"]["text"			] = buffer;

		output["index"				] = total_number_of_images_processed;
		output["duration"			] = nn.duration_string();
		output["tiles"]["horizontal"] = nn.horizontal_tiles;
		output["tiles"]["vertical"	] = nn.vertical_tiles;
		output["tiles"]["width"		] = nn.tile_size.width;
		output["tiles"]["height"	] = nn.tile_size.height;

		if (annotated_filename.empty() == false)
		{
			output["annotated_filename"] = annotated_filename;
		}

		if (txt_filename.empty() == false)
		{
			output["txt_filename"] = txt_filename;
		}

		for (size_t idx = 0; idx < results.size(); idx ++)
		{
			const auto & pred = results[idx];
			auto & j = output["prediction"][idx];

			if (crop_and_save_detected_objects)
			{
				const auto fn = stem + "_idx_" + std::to_string(idx) + "_class_" + std::to_string(pred.best_class) + ".jpg";
				j["crop_filename"]			= fn;
			}

			j["prediction_index"]			= idx;
			j["name"]						= pred.name;
			j["best_class"]					= pred.best_class;
			j["best_probability"]			= pred.best_probability;
			j["original_size"]["width"]		= pred.original_size.width;
			j["original_size"]["height"]	= pred.original_size.height;
			j["original_point"]["x"]		= pred.original_point.x;
			j["original_point"]["y"]		= pred.original_point.y;
			j["rect"]["x"]					= pred.rect.x;
			j["rect"]["y"]					= pred.rect.y;
			j["rect"]["width"]				= pred.rect.width;
			j["rect"]["height"]				= pred.rect.height;

			size_t prop_count = 0;
			for (const auto & prop : pred.all_probabilities)
			{
				j["all_probabilities"][prop_count]["class"			] = prop.first;
				j["all_probabilities"][prop_count]["probability"	] = prop.second;
				j["all_probabilities"][prop_count]["name"			] = nn.names[prop.first];
				prop_count ++;
			}

			if (apply_roi)
			{
				bool roi_found = false;
				for (const auto & r : roi_rectangles)
				{
					const cv::Rect intersection = (r & pred.rect);
					if (intersection.area() > 0.0f)
					{
						// the object detected is in a RoI, so remember this rectangle
						j["roi"]["x"]				= r.x;
						j["roi"]["y"]				= r.y;
						j["roi"]["width"]			= r.width;
						j["roi"]["height"]			= r.height;
						roi_found = true;
						break;
					}
				}

				j["detection_is_in_roi"] = roi_found;
			}
		}

		std::ofstream ofs(stem + ".json");
		ofs << output.dump(4) << std::endl;
	}

	if (crop_and_save_detected_objects)
	{
		for (size_t idx = 0; idx < results.size(); idx ++)
		{
			const auto & prediction = results[idx];
			const auto fn = stem + "_idx_" + std::to_string(idx) + "_class_" + std::to_string(prediction.best_class) + ".jpg";
			cv::imwrite(fn, mat(prediction.rect), {cv::ImwriteFlags::IMWRITE_JPEG_QUALITY, 70});
		}
	}

	return;
}


void server(DarkHelp::NN & nn, const nlohmann::json & j)
{
	const auto & server_settings = j["darkhelp"]["server"]["settings"];

	auto input_dir		= std::filesystem::path(server_settings["input_directory"].get<std::string>());
	auto output_dir		= std::filesystem::path(server_settings["output_directory"].get<std::string>());

	if (server_settings["clear_output_directory_on_startup"])
	{
		std::filesystem::remove_all(output_dir);
	}

	std::filesystem::create_directories(input_dir);
	std::filesystem::create_directories(output_dir);

	std::cout << "-> DarkHelp Server is now running..." << std::endl;

	const auto idle_timeout_in_seconds = std::chrono::seconds(server_settings["idle_time_in_seconds"]);
	const bool exit_if_idle								= server_settings["exit_if_idle"					];
	const int max_images_to_process_at_once				= server_settings["max_images_to_process_at_once"	];
	const bool purge_files_after_cmd_completes			= server_settings["purge_files_after_cmd_completes"	];
	const std::string run_cmd_after_processing_images	= server_settings["run_cmd_after_processing_images"	];
	const bool use_camera_for_input						= server_settings["use_camera_for_input"			];
	crop_and_save_detected_objects						= server_settings["crop_and_save_detected_objects"	];
	save_annotated_image								= server_settings["save_annotated_image"			];
	save_txt_annotations								= server_settings["save_txt_annotations"			];
	save_json_results									= server_settings["save_json_results"				];
	apply_roi											= server_settings["apply_roi"						] ;
	const bool save_original_image						= server_settings["camera"]["save_original_image"	];

	cv::VideoCapture cap;
	if (use_camera_for_input)
	{
		const auto & camera = server_settings["camera"];

		std::string name	= camera["name"			];
		int bufferSize		= camera["buffersize"	];
		int width			= camera["width"		];
		int height			= camera["height"		];
		int fps				= camera["fps"			];

		std::cout << "-> configuring camera device \"" << name << "\" to use " << width << " x " << height << " @ " << fps << " FPS with a buffer size of " << bufferSize << std::endl;
		if (name.empty() == false and name[0] >= '0' and name[0] <= '9')
		{
			int index = std::stoi(name);
			cap.open(index);
		}
		else
		{
			cap.open(name);
		}
		if (cap.isOpened() == false)
		{
			throw std::invalid_argument("failed to open camera device " + name);
		}
		cap.set(cv::VideoCaptureProperties::CAP_PROP_BUFFERSIZE		, bufferSize	);
		cap.set(cv::VideoCaptureProperties::CAP_PROP_FRAME_WIDTH	, width			);
		cap.set(cv::VideoCaptureProperties::CAP_PROP_FRAME_HEIGHT	, height		);
		cap.set(cv::VideoCaptureProperties::CAP_PROP_FPS			, fps			);

		cv::Mat mat;
		cap >> mat;
		if (mat.empty())
		{
			std::cout << "WARNING: reading from camera device " << name << " is returning empty frames" << std::endl;
		}

		bufferSize	= cap.get(cv::VideoCaptureProperties::CAP_PROP_BUFFERSIZE	);
		width		= cap.get(cv::VideoCaptureProperties::CAP_PROP_FRAME_WIDTH	);
		height		= cap.get(cv::VideoCaptureProperties::CAP_PROP_FRAME_HEIGHT	);
		fps			= cap.get(cv::VideoCaptureProperties::CAP_PROP_FPS			);
		std::cout << "-> camera device " + name + " is reporting " << width << " x " << height << " @ " << fps << " FPS with a buffer size of " << bufferSize << std::endl;
		std::cout << "-> actual frame from camera device " + name + " measures " << mat.cols << " x " << mat.rows << std::endl;
	}
	else
	{
		std::cout << "-> reading images from directory " << input_dir.string() << std::endl;
	}

	int images_processed = 0;
	std::filesystem::directory_iterator dir_iter;

	while (true)
	{
		const auto now = std::chrono::high_resolution_clock::now();

		if (exit_if_idle and now > last_activity + idle_timeout_in_seconds)
		{
			std::cout << "-> idle timeout detected after " << idle_timeout_in_seconds.count() << " seconds" << std::endl;
			break;
		}

		cv::Mat mat;
		std::string dst_stem;

		if (use_camera_for_input)
		{
			cap >> mat;
			dst_stem = (output_dir / ("frame_" + std::to_string(total_number_of_images_processed))).string();

			if (save_original_image and not mat.empty())
			{
				cv::imwrite(dst_stem + ".jpg", mat, {cv::ImwriteFlags::IMWRITE_JPEG_QUALITY, 70});
			}
		}
		else
		{
			if (dir_iter == std::filesystem::directory_iterator())
			{
				dir_iter = std::filesystem::directory_iterator(
					input_dir														,
					std::filesystem::directory_options::follow_directory_symlink	|
					std::filesystem::directory_options::skip_permission_denied		);
			}

			if (dir_iter != std::filesystem::directory_iterator())
			{
				auto src = dir_iter->path();
				dir_iter ++;

				if (std::filesystem::exists(src) == false)
				{
					// file has since been deleted -- nothing we can do but move on
					if (src.extension() != ".roi")
					{
						std::cout << "skipping (no longer exists?): " << src << std::endl;
					}
					continue;
				}

				std::cout << "-> [" << total_number_of_images_processed << "] " << src.string() << std::endl;
				auto dst = output_dir / src.filename();
				dst_stem = (output_dir / src.stem()).string();

				// on older versions of OpenCV, such as the one from Ubuntu 18.04,
				// cv::imread() will throw instead of returning an empty cv::mat
				try
				{
					mat = cv::imread(src.string());
				}
				catch (...)
				{
//					std::cout << "failed to load image " << src << std::endl;
				}

				if (mat.empty())
				{
					// maybe this is a .roi file?  loop back to the top and see if we can find the image
					continue;
				}

				std::filesystem::rename(src, dst);

				if (apply_roi)
				{
					load_roi(src);
					if (not roi_fn.empty())
					{
						dst.replace_extension(".roi");
						std::filesystem::rename(roi_fn, dst);
					}
				}
			}
		}

		if (mat.empty() == false)
		{
			process_image(nn, mat, dst_stem);
			images_processed ++;
		}

		if ((mat.empty() and images_processed > 0) or
			(max_images_to_process_at_once > 0 and images_processed >= max_images_to_process_at_once))
		{
			static auto previous_timestamp = now;
			if (now > previous_timestamp)
			{
				const double nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(now - previous_timestamp).count();
				const double fps = static_cast<double>(images_processed) / nanoseconds * 1000000000.0;
				std::cout << "-> " << std::fixed << std::setprecision(1) << fps << " FPS" << std::endl;
			}

			if (run_cmd_after_processing_images.empty() == false)
			{
				std::cout << "-> calling script after processing new images: " << images_processed << std::endl;
				const auto rc = system(run_cmd_after_processing_images.c_str());
				if (rc)
				{
					std::cout << "-> WARNING: command returned rc=" << rc << std::endl;
				}

				if (purge_files_after_cmd_completes and rc == 0)
				{
					std::filesystem::remove_all(output_dir);
					std::filesystem::create_directories(output_dir);
				}
			}

			previous_timestamp = now;
			images_processed = 0;
		}

		if (mat.empty())
		{
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	}

	return;
}


int main(int argc, char *argv[])
{
	std::cout << "DarkHelp Server v" << DH_VERSION << std::endl << std::endl;
	int rc = 1;

	try
	{
		const auto default_settings = create_darkhelp_defaults();

		if (argc != 2)
		{
			std::cout
				<< "ERROR: Invalid parameters."							<< std::endl
				<< ""													<< std::endl
				<< "Usage:"												<< std::endl
				<< ""													<< std::endl
				<< "\t" << argv[0] << " <json-config-filename>"			<< std::endl
				<< ""													<< std::endl
				<< "Here is a default json config file you can modify:"	<< std::endl
				<< ""													<< std::endl
				<< default_settings.dump(4)								<< std::endl;
		}
		else
		{
			nlohmann::json user_settings;

			if (true)
			{
				std::cout << "-> reading DarkHelp Server settings from \"" << argv[1] << "\"..." << std::endl;
				std::ifstream ifs(argv[1]);
				user_settings = nlohmann::json::parse(ifs);
			}

			auto settings = merge(default_settings, user_settings);

			std::cout << settings.dump(4) << std::endl;
			for (const auto & msg : messages)
			{
				std::cout << msg << std::endl;
			}
			if (messages.empty() == false)
			{
				throw std::invalid_argument(messages[0]);
			}

			DarkHelp::NN nn;
			configure(nn, settings);
			server(nn, settings);

			rc = 0;
		}
	}
	catch (const std::exception & e)
	{
		std::cout << std::endl << "Exception thrown:" << std::endl << e.what() << std::endl;
		rc = 2;
	}

	std::cout << "-> DarkHelp Server is exiting with rc=" << rc << std::endl;

	return rc;
}
