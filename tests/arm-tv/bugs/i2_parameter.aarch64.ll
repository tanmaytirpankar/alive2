; ModuleID = 'M2'
source_filename = "M2"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-linux-gnu"

define <2 x i64> @ext_i2_2i64(i2 %0) {
  %2 = bitcast i2 %0 to <2 x i1>
  %3 = sext <2 x i1> %2 to <2 x i64>
  ret <2 x i64> %3
}
