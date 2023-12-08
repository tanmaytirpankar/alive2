define void @tb_l(ptr %0, ptr %1) {
  %3 = load <16 x i8>, ptr %0, align 16
  %4 = load <16 x i8>, ptr %1, align 16
  %5 = extractelement <16 x i8> %3, i32 8
  %6 = extractelement <16 x i8> %4, i32 8
  %7 = extractelement <16 x i8> %3, i32 9
  %8 = extractelement <16 x i8> %4, i32 9
  %9 = extractelement <16 x i8> %3, i32 10
  %10 = extractelement <16 x i8> %4, i32 10
  %11 = extractelement <16 x i8> %3, i32 11
  %12 = extractelement <16 x i8> %4, i32 11
  %13 = extractelement <16 x i8> %3, i32 12
  %14 = extractelement <16 x i8> %4, i32 12
  %15 = extractelement <16 x i8> %3, i32 13
  %16 = extractelement <16 x i8> %4, i32 13
  %17 = extractelement <16 x i8> %3, i32 14
  %18 = extractelement <16 x i8> %4, i32 14
  %19 = extractelement <16 x i8> %3, i32 15
  %20 = extractelement <16 x i8> %4, i32 15
  %21 = insertelement <16 x i8> undef, i8 %5, i32 0
  %22 = insertelement <16 x i8> %21, i8 %6, i32 1
  %23 = insertelement <16 x i8> %22, i8 %7, i32 2
  %24 = insertelement <16 x i8> %23, i8 %8, i32 3
  %25 = insertelement <16 x i8> %24, i8 %9, i32 4
  %26 = insertelement <16 x i8> %25, i8 %10, i32 5
  %27 = insertelement <16 x i8> %26, i8 %11, i32 6
  %28 = insertelement <16 x i8> %27, i8 %12, i32 7
  %29 = insertelement <16 x i8> %28, i8 %13, i32 8
  %30 = insertelement <16 x i8> %29, i8 %14, i32 9
  %31 = insertelement <16 x i8> %30, i8 %15, i32 10
  %32 = insertelement <16 x i8> %31, i8 %16, i32 11
  %33 = insertelement <16 x i8> %32, i8 %17, i32 12
  %34 = insertelement <16 x i8> %33, i8 %18, i32 13
  %35 = insertelement <16 x i8> %34, i8 %19, i32 14
  %36 = insertelement <16 x i8> %35, i8 %20, i32 15
  store <16 x i8> %36, ptr %0, align 16
  ret void
}
