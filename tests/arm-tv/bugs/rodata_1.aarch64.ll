; ModuleID = 'reduced.bc'
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128"
target triple = "arm64-apple-macosx14.0.0"

; Function Attrs: strictfp
define void @_GLOBAL__sub_I_grid_reordering.cc(ptr %_ZN6dealii8internal16GridReordering3d11ElementInfoL19edge_to_node_orientE) #0 section "__TEXT,__StaticInit,regular,pure_instructions" {
entry:
  store <16 x i8> <i8 102, i8 102, i8 102, i8 98, i8 102, i8 102, i8 98, i8 98, i8 102, i8 102, i8 98, i8 102, i8 102, i8 102, i8 98, i8 98>, ptr %_ZN6dealii8internal16GridReordering3d11ElementInfoL19edge_to_node_orientE, align 16
  ret void
}

attributes #0 = { strictfp }
