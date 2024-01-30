; ModuleID = 'reduced.bc'
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128"
target triple = "arm64-apple-macosx14.0.0"

; Function Attrs: strictfp
define i32 @htab_ct_eq(ptr %of1, ptr %of2) #0 {
entry:
  %call = tail call i32 @memcmp(ptr %of2, ptr %of1, i64 1) #0
  ret i32 %call
}

declare i32 @memcmp(ptr, ptr, i64)

attributes #0 = { strictfp }
