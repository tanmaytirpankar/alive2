define <16 x i8> @f(<4 x i32> %0, <4 x i32> %1, <4 x i32> %2, <4 x i32> %3) {
  %5 = extractelement <4 x i32> %0, i32 0
  %6 = extractelement <4 x i32> %0, i32 1
  %7 = extractelement <4 x i32> %0, i32 2
  %8 = extractelement <4 x i32> %0, i32 3
  %9 = extractelement <4 x i32> %1, i32 0
  %10 = extractelement <4 x i32> %1, i32 1
  %11 = extractelement <4 x i32> %1, i32 2
  %12 = extractelement <4 x i32> %1, i32 3
  %13 = extractelement <4 x i32> %2, i32 0
  %14 = extractelement <4 x i32> %2, i32 1
  %15 = extractelement <4 x i32> %2, i32 2
  %16 = extractelement <4 x i32> %2, i32 3
  %17 = extractelement <4 x i32> %3, i32 0
  %18 = extractelement <4 x i32> %3, i32 1
  %19 = extractelement <4 x i32> %3, i32 2
  %20 = extractelement <4 x i32> %3, i32 3
  %21 = bitcast i32 %5 to <2 x i16>
  %22 = bitcast i32 %6 to <2 x i16>
  %23 = bitcast i32 %7 to <2 x i16>
  %24 = bitcast i32 %8 to <2 x i16>
  %25 = bitcast i32 %9 to <2 x i16>
  %26 = bitcast i32 %10 to <2 x i16>
  %27 = bitcast i32 %11 to <2 x i16>
  %28 = bitcast i32 %12 to <2 x i16>
  %29 = bitcast i32 %13 to <2 x i16>
  %30 = bitcast i32 %14 to <2 x i16>
  %31 = bitcast i32 %15 to <2 x i16>
  %32 = bitcast i32 %16 to <2 x i16>
  %33 = bitcast i32 %17 to <2 x i16>
  %34 = bitcast i32 %18 to <2 x i16>
  %35 = bitcast i32 %19 to <2 x i16>
  %36 = bitcast i32 %20 to <2 x i16>
  %37 = shufflevector <2 x i16> %21, <2 x i16> %22, <2 x i32> <i32 1, i32 3>
  %38 = shufflevector <2 x i16> %23, <2 x i16> %24, <2 x i32> <i32 1, i32 3>
  %39 = shufflevector <2 x i16> %25, <2 x i16> %26, <2 x i32> <i32 1, i32 3>
  %40 = shufflevector <2 x i16> %27, <2 x i16> %28, <2 x i32> <i32 1, i32 3>
  %41 = shufflevector <2 x i16> %29, <2 x i16> %30, <2 x i32> <i32 1, i32 3>
  %42 = shufflevector <2 x i16> %31, <2 x i16> %32, <2 x i32> <i32 1, i32 3>
  %43 = shufflevector <2 x i16> %33, <2 x i16> %34, <2 x i32> <i32 1, i32 3>
  %44 = shufflevector <2 x i16> %35, <2 x i16> %36, <2 x i32> <i32 1, i32 3>
  %45 = bitcast <2 x i16> %37 to <4 x i8>
  %46 = bitcast <2 x i16> %38 to <4 x i8>
  %47 = bitcast <2 x i16> %39 to <4 x i8>
  %48 = bitcast <2 x i16> %40 to <4 x i8>
  %49 = bitcast <2 x i16> %41 to <4 x i8>
  %50 = bitcast <2 x i16> %42 to <4 x i8>
  %51 = bitcast <2 x i16> %43 to <4 x i8>
  %52 = bitcast <2 x i16> %44 to <4 x i8>
  %53 = shufflevector <4 x i8> %45, <4 x i8> %46, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %54 = shufflevector <4 x i8> %47, <4 x i8> %48, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %55 = shufflevector <4 x i8> %49, <4 x i8> %50, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %56 = shufflevector <4 x i8> %51, <4 x i8> %52, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %57 = shufflevector <4 x i8> %53, <4 x i8> %54, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %58 = shufflevector <4 x i8> %55, <4 x i8> %56, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %59 = shufflevector <8 x i8> %57, <8 x i8> %58, <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  ret <16 x i8> %59
}