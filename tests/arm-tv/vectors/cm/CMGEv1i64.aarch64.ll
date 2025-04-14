target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "aarch64-unknown-linux-gnu"

define void @test(i64 %var_0, i64 %var_5, ptr %var_21) {
entry:
  %cmp46.not = icmp sle i64 %var_5, %var_0
  %0 = select i1 %cmp46.not, <4 x i1> splat (i1 true), <4 x i1> zeroinitializer
  %1 = extractelement <4 x i1> %0, i64 0
  %2 = zext i1 %1 to i8
  store i8 %2, ptr %var_21, align 1
  ret void
}
