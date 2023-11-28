define void @f2(i64 %0, i64 %1, ptr %2, <2 x i64> %3) {
  %5 = trunc i64 %0 to i32
  %6 = insertelement <2 x i64> %3, i64 %0, i32 0
  %7 = insertelement <2 x i64> %6, i64 %1, i32 %5
  %8 = bitcast <2 x i64> %7 to i128
  %9 = add i128 %8, 1
  store i128 %9, ptr %2, align 4
  ret void
}
