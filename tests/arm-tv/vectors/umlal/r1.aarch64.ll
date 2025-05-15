target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "aarch64-unknown-linux-gnu"

define void @test(i1 %0, ptr %p) {
entry:
  %broadcast.splatinsert741 = insertelement <16 x i16> splat (i16 1), i16 0, i64 0
  %broadcast.splat742 = shufflevector <16 x i16> %broadcast.splatinsert741, <16 x i16> zeroinitializer, <16 x i32> zeroinitializer
  %1 = zext <16 x i16> %broadcast.splat742 to <16 x i32>
  br label %vector.body731

vector.body731:                                   ; preds = %vector.body731, %entry
  %vec.phi734 = phi <16 x i32> [ zeroinitializer, %entry ], [ %4, %vector.body731 ]
  %wide.vec745 = load <32 x i16>, ptr %p, align 2
  %strided.vec746 = shufflevector <32 x i16> %wide.vec745, <32 x i16> zeroinitializer, <16 x i32> <i32 0, i32 2, i32 4, i32 6, i32 8, i32 10, i32 12, i32 14, i32 16, i32 18, i32 20, i32 22, i32 24, i32 26, i32 28, i32 30>
  %2 = zext <16 x i16> %strided.vec746 to <16 x i32>
  %3 = mul <16 x i32> %2, %1
  %4 = sub <16 x i32> %vec.phi734, %3
  br i1 %0, label %middle.block748, label %vector.body731

middle.block748:                                  ; preds = %vector.body731
  %bin.rdx749 = or <16 x i32> %vec.phi734, zeroinitializer
  ret void
}
