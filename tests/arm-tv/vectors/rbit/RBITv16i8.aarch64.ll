define void @bitreverse_v32i8(ptr %0) {
  %2 = load <32 x i8>, ptr %0, align 32
  %3 = call <32 x i8> @llvm.bitreverse.v32i8(<32 x i8> %2)
  store <32 x i8> %3, ptr %0, align 32
  ret void
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare <32 x i8> @llvm.bitreverse.v32i8(<32 x i8>) #1
