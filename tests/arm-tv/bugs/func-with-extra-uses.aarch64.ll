target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128"
target triple = "arm64-apple-macosx14.0.0"

@_ZTV8cMessage = constant { [28 x ptr] } { [28 x ptr] [ptr null, ptr null, ptr null, ptr null, ptr null, ptr null, ptr null, ptr null, ptr null, ptr null, ptr null, ptr null, ptr null, ptr null, ptr null, ptr null, ptr null, ptr null, ptr null, ptr null, ptr @_ZN8cMessage12forEachChildEP8cVisitor, ptr null, ptr null, ptr null, ptr null, ptr null, ptr null, ptr null] }

; Function Attrs: strictfp
define void @_ZN8cMessage12forEachChildEP8cVisitor() #0 {
entry:
  ret void
}

attributes #0 = { strictfp }
