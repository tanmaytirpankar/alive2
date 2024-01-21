; ModuleID = 'reduced.bc'
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128"
target triple = "arm64-apple-macosx14.0.0"

; Function Attrs: strictfp
define ptr @_ZN8cMessageaSERKS_() #0 {
entry:
  br label %return

if.then10:                                        ; No predecessors!
  %call12 = tail call ptr null(ptr null) #0
  br label %return

return:                                           ; preds = %if.then10, %entry
  ret ptr null
}

attributes #0 = { strictfp }
