target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128"
target triple = "arm64-apple-macosx14.0.0"

@.str.15 = private constant [1 x i8] zeroinitializer

; Function Attrs: strictfp
define ptr @_ZNK8cMessage16getDisplayStringEv() #0 {
entry:
  ret ptr @.str.15
}

attributes #0 = { strictfp }
