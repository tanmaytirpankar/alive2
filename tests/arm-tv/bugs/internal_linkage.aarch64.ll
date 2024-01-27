; ModuleID = 'reduced.bc'
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128"
target triple = "arm64-apple-macosx14.0.0"

%struct.arc = type { i32, i64, ptr, ptr, i16, ptr, ptr, i64, i64 }

; Function Attrs: nocallback nofree nounwind willreturn memory(argmem: readwrite)
declare void @llvm.memcpy.p0.p0.i64(ptr noalias nocapture writeonly, ptr noalias nocapture readonly, i64, i1 immarg) #0

declare ptr @malloc()

; Function Attrs: strictfp
define internal i32 @arc_compare(ptr %a1, ptr %a2) #1 {
entry:
  %0 = load ptr, ptr %a1, align 8
  %flow = getelementptr %struct.arc, ptr %0, i64 0, i32 7
  %1 = load i64, ptr %a2, align 8
  %2 = load ptr, ptr %a1, align 8
  %flow1 = getelementptr %struct.arc, ptr %2, i64 0, i32 7
  %3 = load i64, ptr %a2, align 8
  %cmp = icmp sgt i64 0, 0
  %cmp4 = icmp slt i64 0, 0
  ret i32 0
}

attributes #0 = { nocallback nofree nounwind willreturn memory(argmem: readwrite) }
attributes #1 = { strictfp }
