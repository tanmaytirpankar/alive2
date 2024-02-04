; ModuleID = 'reduced.bc'
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128"
target triple = "arm64-apple-macosx14.0.0"

declare double @_ZN3pov5NoiseEPdPNS_14Pattern_StructE()

; Function Attrs: strictfp
define double @_ZN3pov8pattern3EPdPNS_14Pattern_StructE() #0 {
entry:
  %call = call double @_ZN3pov5NoiseEPdPNS_14Pattern_StructE()
  ret double %call
}

attributes #0 = { strictfp }
