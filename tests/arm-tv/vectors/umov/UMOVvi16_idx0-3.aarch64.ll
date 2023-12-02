define <4 x i16> @f(<4 x i16> %0, <4 x i16> %1) {
  %3 = extractelement <4 x i16> %0, i64 0
  %4 = extractelement <4 x i16> %0, i64 1
  %5 = extractelement <4 x i16> %0, i64 2
  %6 = extractelement <4 x i16> %0, i64 3
  %7 = extractelement <4 x i16> %1, i64 0
  %8 = extractelement <4 x i16> %1, i64 1
  %9 = extractelement <4 x i16> %1, i64 2
  %10 = extractelement <4 x i16> %1, i64 3
  %11 = call i16 @llvm.uadd.sat.i16(i16 %3, i16 %7)
  %12 = call i16 @llvm.uadd.sat.i16(i16 %4, i16 %8)
  %13 = call i16 @llvm.uadd.sat.i16(i16 %5, i16 %9)
  %14 = call i16 @llvm.uadd.sat.i16(i16 %6, i16 %10)
  %15 = insertelement <4 x i16> undef, i16 %11, i64 0
  %16 = insertelement <4 x i16> %15, i16 %12, i64 1
  %17 = insertelement <4 x i16> %16, i16 %13, i64 2
  %18 = insertelement <4 x i16> %17, i16 %14, i64 3
  ret <4 x i16> %18
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i16 @llvm.uadd.sat.i16(i16, i16) #0
