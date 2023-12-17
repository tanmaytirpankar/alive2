define void @abs_v16i16(ptr %0) {
  %2 = load <16 x i16>, ptr %0, align 32
  %3 = call <16 x i16> @llvm.abs.v16i16(<16 x i16> %2, i1 false)
  store <16 x i16> %3, ptr %0, align 32
  ret void
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare <16 x i16> @llvm.abs.v16i16(<16 x i16>, i1 immarg) #0
