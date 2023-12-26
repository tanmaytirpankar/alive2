; ModuleID = '<stdin>'
source_filename = "<stdin>"

@v2 = external global <4 x i32>, align 16
@v3 = external global <4 x i32>, align 16
@x = external global <4 x i32>, align 16

; Function Attrs: nounwind
define void @testCombineMultiplies_non_splat(<4 x i32> %0) {
  %2 = add <4 x i32> %0, <i32 11, i32 22, i32 33, i32 44>
  %3 = mul <4 x i32> %2, <i32 22, i32 33, i32 44, i32 55>
  %4 = add <4 x i32> %0, <i32 33, i32 44, i32 55, i32 66>
  %5 = mul <4 x i32> %4, <i32 22, i32 33, i32 44, i32 55>
  store <4 x i32> %3, ptr @v2, align 16
  store <4 x i32> %5, ptr @v3, align 16
  store <4 x i32> %2, ptr @x, align 16
  ret void
}


