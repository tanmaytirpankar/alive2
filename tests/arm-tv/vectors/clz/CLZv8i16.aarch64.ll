declare <16 x i16> @llvm.cttz.v16i16(<16 x i16>, i1 immarg) #0

; Function Attrs: nounwind
define void @cttz_zero_undef_v16i16(ptr %0, ptr %1) {
  %3 = load <16 x i16>, ptr %0, align 32
  %4 = load <16 x i16>, ptr %1, align 32
  %5 = call <16 x i16> @llvm.cttz.v16i16(<16 x i16> %3, i1 true)
  store <16 x i16> %5, ptr %0, align 32
  ret void
}
