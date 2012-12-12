# Compile WebP internally

include_directories( ${WEBP_DIR} )

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release/${GAMELIB_OUTPUT_DIR}" )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug/${GAMELIB_OUTPUT_DIR}" )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/MinSizeRel/${GAMELIB_OUTPUT_DIR}" )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/RelWithDebInfo/${GAMELIB_OUTPUT_DIR}" )

set( WEBPLIST
${WEBP_DIR}/dsp/lossless.c
${WEBP_DIR}/dsp/yuv.c
${WEBP_DIR}/dsp/cpu.c
${WEBP_DIR}/dsp/dec_neon.c
${WEBP_DIR}/dsp/upsampling.c
${WEBP_DIR}/dsp/upsampling_sse2.c
${WEBP_DIR}/dsp/lossless.h
${WEBP_DIR}/dsp/enc_sse2.c
${WEBP_DIR}/dsp/dec_sse2.c
${WEBP_DIR}/dsp/dec.c
${WEBP_DIR}/dsp/dsp.h
${WEBP_DIR}/dsp/enc.c
${WEBP_DIR}/dsp/yuv.h
${WEBP_DIR}/dec/frame.c
${WEBP_DIR}/dec/alpha.c
${WEBP_DIR}/dec/tree.c
${WEBP_DIR}/dec/io.c
${WEBP_DIR}/dec/quant.c
${WEBP_DIR}/dec/vp8.c
${WEBP_DIR}/dec/vp8li.h
${WEBP_DIR}/dec/webp.c
${WEBP_DIR}/dec/buffer.c
${WEBP_DIR}/dec/idec.c
${WEBP_DIR}/dec/vp8l.c
${WEBP_DIR}/dec/vp8i.h
${WEBP_DIR}/dec/layer.c
${WEBP_DIR}/dec/webpi.h
${WEBP_DIR}/dec/decode_vp8.h
${WEBP_DIR}/utils/utils.h
${WEBP_DIR}/utils/rescaler.c
${WEBP_DIR}/utils/bit_writer.c
${WEBP_DIR}/utils/quant_levels.c
${WEBP_DIR}/utils/color_cache.h
${WEBP_DIR}/utils/huffman_encode.c
${WEBP_DIR}/utils/color_cache.c
${WEBP_DIR}/utils/huffman.h
${WEBP_DIR}/utils/utils.c
${WEBP_DIR}/utils/thread.h
${WEBP_DIR}/utils/filters.c
${WEBP_DIR}/utils/quant_levels.h
${WEBP_DIR}/utils/huffman.c
${WEBP_DIR}/utils/thread.c
${WEBP_DIR}/utils/bit_reader.h
${WEBP_DIR}/utils/bit_reader.c
${WEBP_DIR}/utils/bit_writer.h
${WEBP_DIR}/utils/rescaler.h
${WEBP_DIR}/utils/filters.h
${WEBP_DIR}/utils/huffman_encode.h
${WEBP_DIR}/webp/types.h
${WEBP_DIR}/webp/decode.h
${WEBP_DIR}/webp/mux.h
${WEBP_DIR}/webp/format_constants.h
${WEBP_DIR}/webp/encode.h
)

add_library( INTERNAL_WEBP STATIC ${WEBPLIST} )
set_property( TARGET INTERNAL_WEBP PROPERTY COMPILE_FLAGS "-fPIC" )

