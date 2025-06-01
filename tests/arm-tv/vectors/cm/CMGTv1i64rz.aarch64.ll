target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "aarch64-unknown-linux-gnu"

define <1 x i64> @test(<1 x i64> %0) {
entry:
  %1 = tail call <1 x i64> @llvm.smax.v1i64(<1 x i64> %0, <1 x i64> zeroinitializer)
  ret <1 x i64> %1
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare <1 x i64> @llvm.smax.v1i64(<1 x i64>, <1 x i64>) #0

attributes #0 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }
