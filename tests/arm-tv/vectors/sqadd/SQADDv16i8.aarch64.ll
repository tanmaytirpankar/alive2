declare <16 x i8> @llvm.sadd.sat.v16i8(<16 x i8>, <16 x i8>) #0

define <16 x i8> @f(<16 x i8> %0) {
  %2 = insertelement <16 x i8> poison, i8 5, i32 0
  %3 = shufflevector <16 x i8> %2, <16 x i8> poison, <16 x i32> zeroinitializer
  %4 = call <16 x i8> @llvm.sadd.sat.v16i8(<16 x i8> %0, <16 x i8> %3)
  ret <16 x i8> %4
}
