; ModuleID = 'reduced.bc'
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128"
target triple = "arm64-apple-macosx14.0.0"

; Function Attrs: strictfp
define ptr @f() #0 {
entry:
  %call = call ptr @png_create_write_struct_2()
  ret ptr %call
}

; Function Attrs: strictfp
declare noalias ptr @png_create_write_struct_2() #0

attributes #0 = { strictfp }
