declare <16 x i8> @llvm.umax.v16i8(<16 x i8>, <16 x i8>) #0

define void @umax_xv_v16i8(ptr %0, i8 %1) {
  %3 = load <16 x i8>, ptr %0, align 16
  %4 = insertelement <16 x i8> poison, i8 %1, i32 0
  %5 = shufflevector <16 x i8> %4, <16 x i8> poison, <16 x i32> zeroinitializer
  %6 = call <16 x i8> @llvm.umax.v16i8(<16 x i8> %5, <16 x i8> %3)
  store <16 x i8> %6, ptr %0, align 16
  ret void
}
