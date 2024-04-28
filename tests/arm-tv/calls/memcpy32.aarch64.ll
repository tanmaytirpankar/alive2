define ptr @f() {
  call void @llvm.memcpy.element.unordered.atomic.p0.p0.i32(ptr align 4 null, ptr align 4 null, i32 0, i32 1)
  ret ptr null
}

; Function Attrs: nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.memcpy.element.unordered.atomic.p0.p0.i32(ptr nocapture writeonly, ptr nocapture readonly, i32, i32 immarg) #0

attributes #0 = { nosync nounwind willreturn memory(argmem: readwrite) }
