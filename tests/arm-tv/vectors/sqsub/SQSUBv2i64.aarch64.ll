declare { <2 x i64>, <2 x i1> } @llvm.ssub.with.overflow.v2i64(<2 x i64>, <2 x i64>) #0

; Function Attrs: nounwind
define <2 x i32> @f(<2 x i64> %0, <2 x i64> %1, ptr %2) {
  %4 = call { <2 x i64>, <2 x i1> } @llvm.ssub.with.overflow.v2i64(<2 x i64> %0, <2 x i64> %1)
  %5 = extractvalue { <2 x i64>, <2 x i1> } %4, 0
  %6 = extractvalue { <2 x i64>, <2 x i1> } %4, 1
  %7 = sext <2 x i1> %6 to <2 x i32>
  store <2 x i64> %5, ptr %2, align 16
  ret <2 x i32> %7
}
