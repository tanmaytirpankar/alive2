; ModuleID = 'reduced.bc'
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128"
target triple = "arm64-apple-macosx14.0.0"

; Function Attrs: strictfp
declare i32 @_Z7qsearchP7state_tiiii(ptr, i32, i32, i32, i32) #0

; Function Attrs: strictfp
define i32 @_Z6searchP7state_tiiiii() #0 {
entry:
  %call = tail call i32 @_Z7qsearchP7state_tiiii(ptr null, i32 0, i32 0, i32 0, i32 0) #0
  ret i32 %call
}

attributes #0 = { strictfp }
