#
# CMakeLists.txt
#
# cmake required (http://www.cmake.org/)
#
# glsl-optimiser project. Requires it's own CMakeLists.txt file to prevent incorrect
# headers from being included

project(glsl-optimizer C)

######################
# Define Build Files #
######################

set( GLCPPLIST
  ${LIB_DIR}/glsl-optimizer/src/glsl/glcpp/glcpp-lex.c
  ${LIB_DIR}/glsl-optimizer/src/glsl/glcpp/glcpp-parse.c
  ${LIB_DIR}/glsl-optimizer/src/glsl/glcpp/pp.c
)

set( MESALIST
  ${LIB_DIR}/glsl-optimizer/src/mesa/program/hash_table.c
  ${LIB_DIR}/glsl-optimizer/src/mesa/program/symbol_table.c
)

set( GLSLLIST
  ${LIB_DIR}/glsl-optimizer/src/glsl/ast_expr.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ast_function.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ast_to_hir.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ast_type.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/builtin_function.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/builtin_stubs.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/builtin_variables.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/glsl_lexer.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/glsl_optimizer.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/glsl_parser.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/glsl_parser_extras.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/glsl_symbol_table.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/glsl_types.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/hir_field_selection.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ir_basic_block.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ir_builder.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ir_clone.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ir_constant_expression.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ir.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ir_expression_flattening.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ir_function_can_inline.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ir_function.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ir_function_detect_recursion.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ir_hierarchical_visitor.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ir_hv_accept.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ir_import_prototypes.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ir_print_glsl_visitor.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ir_print_visitor.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ir_reader.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ir_rvalue_visitor.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ir_set_program_inouts.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ir_unused_structs.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ir_validate.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ir_variable_refcount.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/linker.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/link_functions.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/link_uniform_initializers.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/link_uniforms.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/loop_analysis.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/loop_controls.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/loop_unroll.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/lower_clip_distance.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/lower_discard.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/lower_discard_flow.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/lower_if_to_cond_assign.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/lower_instructions.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/lower_jumps.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/lower_mat_op_to_vec.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/lower_noise.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/lower_output_reads.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/lower_ubo_reference.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/lower_variable_index_to_cond_assign.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/lower_vec_index_to_cond_assign.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/lower_vec_index_to_swizzle.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/lower_vector.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/main.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/opt_algebraic.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/opt_array_splitting.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/opt_constant_folding.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/opt_constant_propagation.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/opt_constant_variable.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/opt_copy_propagation.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/opt_copy_propagation_elements.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/opt_dead_code.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/opt_dead_code_local.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/opt_dead_functions.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/opt_function_inlining.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/opt_if_simplification.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/opt_noop_swizzle.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/opt_redundant_jumps.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/opt_structure_splitting.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/opt_swizzle_swizzle.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/opt_tree_grafting.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ralloc.c
  ${LIB_DIR}/glsl-optimizer/src/glsl/s_expression.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/standalone_scaffolding.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/strtod.c
)

##################################
# Build GLSL Optimizer Libraries #
##################################

include_directories( BEFORE
  include
  src/mesa
  src/mapi
  src/glsl
)
if( MSVC )
  include_directories( ${LIB_DIR}/glsl-optimizer/include/c99 )
endif()

add_library( glcpp-library ${GLCPPLIST} )
set_target_properties( glcpp-library PROPERTIES COMPILE_FLAGS "-fPIC" )

add_library( mesa ${MESALIST} )
set_target_properties( mesa PROPERTIES COMPILE_FLAGS "-fPIC" )

add_library( glsl_optimizer ${GLSLLIST} )
set_target_properties( glsl_optimizer PROPERTIES COMPILE_FLAGS "-fPIC" )
target_link_libraries( glsl_optimizer glcpp-library mesa )