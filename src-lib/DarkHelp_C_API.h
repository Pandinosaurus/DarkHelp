/* DarkHelp - C++ helper class for Darknet's C API.
 * Copyright 2019-2023 Stephane Charette <stephanecharette@gmail.com>
 * MIT license applies.  See "license.txt" for details.
 */

#pragma once

/** @file
 * This is the @p C API for %DarkHelp.  It exists primarily to help with the Python bindings.  If you have a choice
 * between using the @p C API and the @p C++ one, note the @p C++ API has additional functionality that is not
 * exposed in the @p C or @p Python API.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdint.h>

/** The @p DarkHelpPtr type is only used by the @p C and @p Python APIs.  It is a @ref DarkHelp::NN C++ object which
 * has been cast to a generic C-style @p void* type.
 * @see @ref CreateDarkHelpNN()
 * @see @ref DestroyDarkHelpNN()
 * @since December 2023
 * @note This is part of the @ref CAPI, which is a simplified interface to the full @ref API.
 */
typedef void* DarkHelpPtr;

/** Get the %DarkHelp version string.  @see @ref DarkHelp::version()
 * @since December 2023
 * @note This is part of the @ref CAPI, which is a simplified interface to the full @ref API.
 */
const char * DarkHelpVersion();

/** Get the Darknet version string.  @see @ref DarkHelpVersion()
 * @since December 2023
 * @note This is part of the @ref CAPI, which is a simplified interface to the full @ref API.
 */
const char * DarknetVersion();

/** Calls @ref DarkHelp::toggle_output_redirection() to either hide or show output on @p STDOUT and @p STDERR.
 * This can be used to hide the large amount of output generated by @p Darknet when it first loads a neural network.
 * @since December 2023
 * @note This is part of the @ref CAPI, which is a simplified interface to the full @ref API.
 */
void ToggleOutputRedirection();

/** Create a @ref DarkHelp::NN object.  The order in which the filenames are specified is not important.  The files
 * should be the @p .cfg file, the @p .names file, and the @p .weights file.
 *
 * When done with the neural network, remember to call @ref DestroyDarkHelpNN().
 * @since December 2023
 * @note This is part of the @ref CAPI, which is a simplified interface to the full @ref API.
 */
DarkHelpPtr CreateDarkHelpNN(const char * const fn1, const char * const fn2, const char * const fn3);

/** Destroy the neural network object previously created with @ref CreateDarkHelpNN().
 * @since December 2023
 * @note This is part of the @ref CAPI, which is a simplified interface to the full @ref API.
 */
void DestroyDarkHelpNN(DarkHelpPtr ptr);

/** Calls @ref DarkHelp::NN::predict() with the given image filename.
 * @returns the number of predictions made.
 * @since December 2023
 * @note This is part of the @ref CAPI, which is a simplified interface to the full @ref API.
 */
int PredictFN(DarkHelpPtr ptr, const char * const image_filename);

/** Calls @ref DarkHelp::NN::predict() with the given image.  The image data is presumed to be similar to OpenCV's
 * @p cv::Mat objects, meaning the bytes need to be in BGR format, not RGB.  The number of channels will automatically
 * be determined by dividing the number of bytes by the width and height.
 * @returns the number of predictions made.
 * @since December 2023
 * @note This is part of the @ref CAPI, which is a simplified interface to the full @ref API.
 */
int Predict(DarkHelpPtr ptr, const int width, const int height, uint8_t * image, const int number_of_bytes);

/** Get the last detection results from either @ref PredictFN() or @ref Predict().  The results will be formatted
 * as a JSON string.
 * @note The @p char* buffer used to return the JSON is static, so don't attempt to use this function call simultaneously
 * from multiple threads, otherwise you risk corrupting the returned text or causing strange undefined behaviour.
 * @since December 2023
 * @note This is part of the @ref CAPI, which is a simplified interface to the full @ref API.
 */
const char * GetPredictionResults(DarkHelpPtr ptr);

/** Calls @ref DarkHelp::NN::annotate() on the last image and results from either @ref PredictFN() or @ref Predict().
 * The results will be saved to @p output_image_filename in either @p PNG or @p JPG format depending on the file
 * extension.
 * @warning If the output filename already exists it will be overwritten.
 * @since December 2023
 * @note This is part of the @ref CAPI, which is a simplified interface to the full @ref API.
 */
void Annotate(DarkHelpPtr ptr, const char * const output_image_filename);

/** Set the detection threshold used with @p Darknet and @p DarkHelp.  @see @ref DarkHelp::Config::threshold
 * @returns the previous value.
 * @since December 2023
 * @note This is part of the @ref CAPI, which is a simplified interface to the full @ref API.
 */
float SetThreshold(DarkHelpPtr ptr, float threshold);

/** Set the NMS (non maximal suppression) threshold used with @p Darknet.  @see @ref DarkHelp::Config::non_maximal_suppression_threshold
 * @returns the previous value.
 * @since December 2023
 * @note This is part of the @ref CAPI, which is a simplified interface to the full @ref API.
 */
float SetNonMaximalSuppression(DarkHelpPtr ptr, float nms);

/** Enable or disable @ref DarkHelp::Config::names_include_percentage.
 * @returns the previous value.
 * @since December 2023
 * @note This is part of the @ref CAPI, which is a simplified interface to the full @ref API.
 */
bool EnableNamesIncludePercentage(DarkHelpPtr ptr, bool enabled);

/** Enable or disable @ref DarkHelp::Config::annotation_auto_hide_labels.
 * @returns the previous value.
 * @since December 2023
 * @note This is part of the @ref CAPI, which is a simplified interface to the full @ref API.
 */
bool EnableAnnotationAutoHideLabels(DarkHelpPtr ptr, bool enabled);

/** Enable or disable @ref DarkHelp::Config::annotation_suppress_all_labels.
 * @returns the previous value.
 * @since December 2023
 * @note This is part of the @ref CAPI, which is a simplified interface to the full @ref API.
 */
bool EnableAnnotationSuppressAllLabels(DarkHelpPtr ptr, bool enabled);

/** Set the amount of shading to use when annotating predictions.  @see @ref DarkHelp::Config::annotation_shade_predictions
 * @returns the previous value.
 * @since December 2023
 * @note This is part of the @ref CAPI, which is a simplified interface to the full @ref API.
 */
float SetAnnotationShadePredictions(DarkHelpPtr ptr, float shading);

/** Enable or disable @ref DarkHelp::Config::include_all_names.
 * @returns the previous value.
 * @since December 2023
 * @note This is part of the @ref CAPI, which is a simplified interface to the full @ref API.
 */
bool EnableIncludeAllNames(DarkHelpPtr ptr, bool enabled);

/** Set the font scale to use when annotating predictions.  @see @ref DarkHelp::Config::annotation_font_scale
 * @returns the previous value.
 * @since December 2023
 * @note This is part of the @ref CAPI, which is a simplified interface to the full @ref API.
 */
double SetAnnotationFontScale(DarkHelpPtr ptr, double scale);

/** Set the font thickness to use when annotating predictions.  @see @ref DarkHelp::Config::annotation_font_thickness
 * @returns the previous value.
 * @since December 2023
 * @note This is part of the @ref CAPI, which is a simplified interface to the full @ref API.
 */
int SetAnnotationFontThickness(DarkHelpPtr ptr, int thickness);

/** Set the line thickness to use when annotating predictions.  @see @ref DarkHelp::Config::annotation_line_thickness
 * @returns the previous value.
 * @since December 2023
 * @note This is part of the @ref CAPI, which is a simplified interface to the full @ref API.
 */
int SetAnnotationLineThickness(DarkHelpPtr ptr, int thickness);

/** Enable or disable @ref DarkHelp::Config::annotation_include_duration.
 * @returns the previous value.
 * @since December 2023
 * @note This is part of the @ref CAPI, which is a simplified interface to the full @ref API.
 */
bool EnableAnnotationIncludeDuration(DarkHelpPtr ptr, bool enabled);

/** Enable or disable @ref DarkHelp::Config::annotation_include_timestamp.
 * @returns the previous value.
 * @since December 2023
 * @note This is part of the @ref CAPI, which is a simplified interface to the full @ref API.
 */
bool EnableAnnotationIncludeTimestamp(DarkHelpPtr ptr, bool enabled);

/** Enable or disable @ref DarkHelp::Config::annotation_pixelate_enabled.
 * @returns the previous value.
 * @since December 2023
 * @note This is part of the @ref CAPI, which is a simplified interface to the full @ref API.
 */
bool EnableAnnotationPixelate(DarkHelpPtr ptr, bool enabled);

/** Set the pixelation size to use when @ref EnableAnnotationPixelate() has been enabled.  @see @ref DarkHelp::Config::annotation_pixelate_size
 * @returns the previous value.
 * @since December 2023
 * @note This is part of the @ref CAPI, which is a simplified interface to the full @ref API.
 */
int SetAnnotationPixelateSize(DarkHelpPtr ptr, int size);

/** Enable or disable @ref DarkHelp::Config::enable_tiles.
 * @returns the previous value.
 * @since December 2023
 * @note This is part of the @ref CAPI, which is a simplified interface to the full @ref API.
 */
bool EnableTiles(DarkHelpPtr ptr, bool enabled);

/** Enable or disable @ref DarkHelp::Config::combine_tile_predictions.
 * @returns the previous value.
 * @since December 2023
 * @note This is part of the @ref CAPI, which is a simplified interface to the full @ref API.
 */
bool EnableCombineTilePredictions(DarkHelpPtr ptr, bool enabled);

/** Enable or disable @ref DarkHelp::Config::only_combine_similar_predictions.
 * @returns the previous value.
 * @since December 2023
 * @note This is part of the @ref CAPI, which is a simplified interface to the full @ref API.
 */
bool EnableOnlyCombineSimilarPredictions(DarkHelpPtr ptr, bool enabled);

/** Set the value for @ref DarkHelp::Config::tile_edge_factor.
 * @returns the previous value.
 * @since December 2023
 * @note This is part of the @ref CAPI, which is a simplified interface to the full @ref API.
 */
float SetTileEdgeFactor(DarkHelpPtr ptr, float factor);

/** Set the value for @ref DarkHelp::Config::tile_rect_factor.
 * @returns the previous value.
 * @since December 2023
 * @note This is part of the @ref CAPI, which is a simplified interface to the full @ref API.
 */
float SetTileRectFactor(DarkHelpPtr ptr, float factor);

/** Enable or disable @ref DarkHelp::Config::snapping_enabled.
 * @returns the previous value.
 * @since December 2023
 * @note This is part of the @ref CAPI, which is a simplified interface to the full @ref API.
 */
bool EnableSnapping(DarkHelpPtr ptr, bool enabled);

/** Set the value for @ref DarkHelp::Config::binary_threshold_block_size.
 * @returns the previous value.
 * @since December 2023
 * @note This is part of the @ref CAPI, which is a simplified interface to the full @ref API.
 */
int SetBinaryThresholdBlockSize(DarkHelpPtr ptr, int blocksize);

/** Set the value for @ref DarkHelp::Config::binary_threshold_constant.
 * @returns the previous value.
 * @since December 2023
 * @note This is part of the @ref CAPI, which is a simplified interface to the full @ref API.
 */
double SetBinaryThresholdConstant(DarkHelpPtr ptr, double threshold);

/** Set the value for @ref DarkHelp::Config::snapping_horizontal_tolerance.
 * @returns the previous value.
 * @since December 2023
 * @note This is part of the @ref CAPI, which is a simplified interface to the full @ref API.
 */
int SetSnappingHorizontalTolerance(DarkHelpPtr ptr, int tolerance);

/** Set the value for @ref DarkHelp::Config::snapping_vertical_tolerance.
 * @returns the previous value.
 * @since December 2023
 * @note This is part of the @ref CAPI, which is a simplified interface to the full @ref API.
 */
int SetSnappingVerticalTolerance(DarkHelpPtr ptr, int tolerance);

/** Set the value for @ref DarkHelp::Config::snapping_limit_shrink.
 * @returns the previous value.
 * @since December 2023
 * @note This is part of the @ref CAPI, which is a simplified interface to the full @ref API.
 */
float SetSnappingLimitShrink(DarkHelpPtr ptr, float limit);

/** Set the value for @ref DarkHelp::Config::snapping_limit_grow.
 * @returns the previous value.
 * @since December 2023
 * @note This is part of the @ref CAPI, which is a simplified interface to the full @ref API.
 */
float SetSnappingLimitGrow(DarkHelpPtr ptr, float limit);

/** Enable or disable @ref DarkHelp::Config::use_fast_image_resize.
 * @returns the previous value.
 * @since December 2023
 * @note This is part of the @ref CAPI, which is a simplified interface to the full @ref API.
 */
bool EnableUseFastImageResize(DarkHelpPtr ptr, bool enabled);

#ifdef __cplusplus
}
#endif