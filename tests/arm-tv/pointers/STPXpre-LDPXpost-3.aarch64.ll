; ModuleID = 'M1'
source_filename = "M1"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-linux-gnu"

define noundef i56 @f(i44 noundef signext %0, i24 noundef %1, i20 zeroext %2, i21 %3, i44 %4) {
  %6 = freeze i21 %3
  %7 = zext i21 %6 to i44
  %8 = icmp slt i44 %7, %0
  %9 = sext i24 %1 to i44
  %10 = icmp ule i44 %9, 5150057284452
  %11 = trunc i44 %4 to i6
  %12 = freeze i24 %1
  %13 = trunc i24 %12 to i6
  %14 = trunc i44 %0 to i1
  %15 = select i1 %14, i6 %11, i6 %13
  %16 = freeze i44 %0
  %maskA = trunc i44 -131073 to i6
  %maskB = zext i6 %maskA to i44
  %17 = udiv i44 %16, -131073
  %18 = sext i1 %10 to i55
  %19 = icmp ult i55 %18, 19074
  %20 = trunc i44 %0 to i20
  %maskC = and i20 %20, 31
  %21 = add nuw i20 524287, %20
  %22 = zext i44 %17 to i49
  %23 = zext i6 %15 to i49
  %24 = icmp uge i49 %22, %23
  %25 = zext i1 %8 to i44
  %26 = icmp sgt i44 %25, %17
  %27 = sext i1 %26 to i44
  %maskC1 = and i44 %27, 63
  %28 = ashr i44 16296, %27
  %29 = zext i1 %26 to i9
  %30 = trunc i44 %28 to i1
  %31 = select i1 %30, i9 -2, i9 %29
  %32 = sext i44 %17 to i48
  %33 = zext i1 %24 to i48
  %34 = trunc i21 %3 to i1
  %35 = select i1 %34, i48 %32, i48 %33
  %36 = zext i21 %3 to i44
  %37 = sext i1 %26 to i44
  %38 = trunc i48 %35 to i1
  %39 = select i1 %38, i44 %36, i44 %37
  %40 = zext i1 %19 to i19
  %41 = trunc i20 %21 to i19
  %42 = select i1 false, i19 %40, i19 %41
  %43 = zext i6 %15 to i38
  %44 = freeze i48 %35
  %45 = trunc i48 %44 to i38
  %46 = call { i38, i1 } @llvm.ssub.with.overflow.i38(i38 %43, i38 %45)
  %47 = extractvalue { i38, i1 } %46, 1
  %48 = extractvalue { i38, i1 } %46, 0
  %49 = zext i19 %42 to i44
  %50 = zext i1 %47 to i44
  %51 = trunc i44 %39 to i1
  %52 = select i1 %51, i44 %49, i44 %50
  %53 = trunc i24 %1 to i19
  %54 = call i19 @llvm.ssub.sat.i19(i19 -1, i19 %53)
  %55 = sext i19 %42 to i44
  %56 = sext i1 %8 to i44
  %maskC2 = and i44 %56, 63
  %57 = udiv i44 %55, %56
  %58 = zext i1 %24 to i44
  %59 = sext i20 %21 to i44
  %60 = trunc i6 %15 to i1
  %61 = select i1 %60, i44 %58, i44 %59
  %62 = sext i24 %1 to i44
  %63 = trunc i20 %2 to i1
  %64 = select i1 %63, i44 %52, i44 %62
  %65 = zext i6 %15 to i52
  %66 = zext i20 %21 to i52
  %maskA3 = trunc i52 %66 to i6
  %maskB4 = zext i6 %maskA3 to i52
  %67 = sdiv i52 %65, %66
  %68 = zext i21 %3 to i22
  %maskA5 = trunc i22 -2097152 to i5
  %maskB6 = zext i5 %maskA5 to i22
  %69 = urem i22 %68, -2097152
  %70 = trunc i22 %69 to i13
  %71 = sext i6 %15 to i13
  %72 = trunc i44 %61 to i1
  %73 = select i1 %72, i13 %70, i13 %71
  %74 = trunc i48 %35 to i44
  %75 = zext i19 %54 to i44
  %maskC7 = and i44 %75, 63
  %76 = or i44 %74, %75
  %77 = zext i19 %42 to i44
  %78 = zext i1 %26 to i44
  %79 = icmp sgt i44 %77, %78
  %80 = sext i20 %2 to i40
  %81 = zext i38 %48 to i40
  %82 = trunc i20 %21 to i1
  %83 = select i1 %82, i40 %80, i40 %81
  %84 = trunc i52 %67 to i44
  %85 = sext i1 %79 to i44
  %86 = select i1 false, i44 %84, i44 %85
  %87 = zext i1 %19 to i44
  %88 = zext i1 %8 to i44
  %maskC8 = and i44 %88, 63
  %89 = srem i44 %87, %88
  %90 = zext i24 %1 to i44
  %91 = icmp sge i44 %90, %0
  %92 = zext i1 %47 to i44
  %maskC9 = and i44 %92, 63
  %93 = and i44 %4, %92
  %94 = trunc i44 %76 to i37
  %95 = trunc i44 %0 to i37
  %96 = trunc i44 %0 to i1
  %97 = select i1 %96, i37 %94, i37 %95
  %98 = sext i9 %31 to i54
  %99 = zext i1 %79 to i54
  %100 = trunc i52 %67 to i1
  %101 = select i1 %100, i54 %98, i54 %99
  %102 = sext i13 %73 to i40
  %103 = trunc i44 %89 to i40
  %maskA10 = trunc i40 %103 to i6
  %maskB11 = sext i6 %maskA10 to i40
  %104 = udiv i40 %102, %103
  %105 = sext i1 %26 to i44
  %106 = freeze i52 %67
  %107 = trunc i52 %106 to i44
  %108 = trunc i44 %28 to i1
  %109 = select i1 %108, i44 %105, i44 %107
  %110 = trunc i44 %39 to i43
  %111 = call i43 @llvm.cttz.i43(i43 %110, i1 true)
  %112 = freeze i1 %91
  %113 = sext i1 %112 to i44
  %maskA12 = trunc i44 %113 to i6
  %maskB13 = sext i6 %maskA12 to i44
  %114 = udiv i44 %89, %113
  %115 = freeze i54 %101
  %116 = trunc i54 %115 to i44
  %117 = icmp uge i44 %116, %52
  %118 = trunc i43 %111 to i12
  %119 = trunc i43 %111 to i1
  %120 = select i1 %119, i12 -769, i12 %118
  %121 = zext i9 %31 to i44
  %122 = sext i13 %73 to i44
  %123 = trunc i44 %89 to i1
  %124 = select i1 %123, i44 %121, i44 %122
  %125 = freeze i44 %89
  %126 = trunc i44 %125 to i21
  %127 = trunc i44 %28 to i21
  %128 = trunc i20 %21 to i1
  %129 = select i1 %128, i21 %126, i21 %127
  %130 = sext i43 %111 to i44
  %131 = sext i1 %79 to i44
  %132 = call i44 @llvm.fshr.i44(i44 %130, i44 %131, i44 %86)
  %133 = freeze i24 %1
  %134 = sext i24 %133 to i44
  %135 = sext i1 %91 to i44
  %136 = icmp ne i44 %134, %135
  %137 = trunc i19 %42 to i6
  %138 = trunc i13 %73 to i6
  %139 = call { i6, i1 } @llvm.usub.with.overflow.i6(i6 %137, i6 %138)
  %140 = extractvalue { i6, i1 } %139, 1
  %141 = extractvalue { i6, i1 } %139, 0
  %142 = sext i44 %52 to i54
  %143 = sext i1 %47 to i54
  %144 = icmp sgt i54 %142, %143
  %145 = zext i21 %129 to i44
  %146 = call i44 @llvm.fshr.i44(i44 %17, i44 %145, i44 %93)
  %147 = sext i1 %144 to i44
  %148 = trunc i44 %114 to i1
  %149 = select i1 %148, i44 %147, i44 %28
  %150 = sext i1 %24 to i62
  %151 = sext i44 %76 to i62
  %152 = trunc i44 %146 to i1
  %153 = select i1 %152, i62 %150, i62 %151
  %154 = zext i9 %31 to i44
  %155 = zext i12 %120 to i44
  %156 = sext i1 %47 to i44
  %157 = call i44 @llvm.fshl.i44(i44 %154, i44 %155, i44 %156)
  %158 = zext i44 %89 to i45
  %159 = zext i44 %86 to i45
  %160 = sext i21 %129 to i45
  %161 = call i45 @llvm.fshr.i45(i45 %158, i45 %159, i45 %160)
  %162 = sext i21 %129 to i44
  %163 = call i44 @llvm.smax.i44(i44 %162, i44 %114)
  %164 = zext i21 %129 to i40
  %165 = trunc i44 %0 to i40
  %166 = call { i40, i1 } @llvm.sadd.with.overflow.i40(i40 %164, i40 %165)
  %167 = extractvalue { i40, i1 } %166, 1
  %168 = extractvalue { i40, i1 } %166, 0
  %169 = sext i19 %54 to i44
  %170 = trunc i45 %161 to i44
  %171 = call i44 @llvm.smin.i44(i44 %169, i44 %170)
  %172 = sext i22 %69 to i34
  %173 = trunc i44 %57 to i34
  %maskA14 = trunc i34 %173 to i6
  %maskB15 = sext i6 %maskA14 to i34
  %174 = mul nsw i34 %172, %173
  %175 = sext i13 %73 to i44
  %176 = sext i1 %26 to i44
  %maskC16 = and i44 %176, 63
  %177 = srem i44 %175, %176
  %178 = sext i20 %21 to i44
  %179 = zext i40 %104 to i44
  %180 = icmp eq i44 %178, %179
  %181 = trunc i40 %104 to i10
  %182 = trunc i40 %168 to i10
  %maskA17 = trunc i10 %182 to i4
  %maskB18 = sext i4 %maskA17 to i10
  %183 = sdiv i10 %181, %182
  %184 = trunc i44 %149 to i41
  %185 = sext i1 %91 to i41
  %186 = call i41 @llvm.smax.i41(i41 %184, i41 %185)
  %187 = sext i44 %146 to i48
  %188 = sext i44 %109 to i48
  %189 = icmp sgt i48 %187, %188
  %190 = sext i24 %1 to i44
  %191 = freeze i44 %177
  %maskA19 = trunc i44 %191 to i6
  %maskB20 = sext i6 %maskA19 to i44
  %192 = mul nuw nsw i44 %190, %191
  %193 = sext i44 %109 to i56
  %194 = sext i44 %192 to i56
  %195 = icmp slt i56 %193, %194
  %196 = zext i1 %140 to i44
  %197 = zext i6 %141 to i44
  %198 = call i44 @llvm.fshr.i44(i44 %196, i44 %197, i44 %157)
  %maskA21 = trunc i44 %4 to i6
  %maskB22 = zext i6 %maskA21 to i44
  %199 = sub i44 %89, %4
  %200 = zext i1 %8 to i44
  %201 = freeze i44 %171
  %maskA23 = trunc i44 %201 to i6
  %maskB24 = zext i6 %maskA23 to i44
  %202 = shl nuw i44 %200, %maskB24
  %203 = zext i10 %183 to i44
  %204 = freeze i44 %89
  %205 = icmp sgt i44 %203, %204
  %206 = sext i21 %3 to i63
  %207 = call i63 @llvm.bitreverse.i63(i63 %206)
  %208 = sext i20 %21 to i44
  %209 = freeze i1 %117
  %210 = zext i1 %209 to i44
  %211 = call i44 @llvm.fshl.i44(i44 %192, i44 %208, i44 %210)
  %212 = sext i1 %8 to i44
  %213 = trunc i63 %207 to i44
  %maskC25 = and i44 %213, 63
  %214 = or i44 %212, %213
  %215 = freeze i44 %0
  %maskA26 = trunc i44 %215 to i6
  %maskB27 = sext i6 %maskA26 to i44
  %216 = and i44 %157, %215
  %217 = sext i43 %111 to i44
  %218 = select i1 %117, i44 %17, i44 %217
  %219 = select i1 %91, i44 %218, i44 %202
  %220 = trunc i38 %48 to i10
  %221 = zext i6 %15 to i10
  %maskC28 = and i10 %221, 15
  %222 = urem i10 %220, %221
  %223 = zext i12 %120 to i44
  %224 = sext i13 %73 to i44
  %225 = call i44 @llvm.smax.i44(i44 %223, i44 %224)
  %226 = icmp uge i44 %199, %202
  %227 = trunc i9 %31 to i3
  %228 = trunc i10 %183 to i3
  %229 = icmp sgt i3 %227, %228
  %230 = zext i22 %69 to i44
  %231 = sext i21 %3 to i44
  %232 = call i44 @llvm.fshr.i44(i44 %230, i44 %76, i44 %231)
  %233 = zext i44 %89 to i58
  %234 = trunc i44 %39 to i1
  %235 = select i1 %234, i58 144115188075855870, i58 %233
  %236 = freeze i1 %229
  %237 = zext i1 %236 to i2
  %238 = trunc i40 %168 to i2
  %239 = freeze i2 %238
  %maskA29 = trunc i2 %239 to i1
  %maskB30 = sext i1 %maskA29 to i2
  %240 = xor i2 %237, %238
  %241 = sext i1 %47 to i44
  %242 = trunc i2 %240 to i1
  %243 = select i1 %242, i44 %241, i44 %0
  %244 = zext i1 %189 to i44
  %maskC31 = and i44 %93, 63
  %245 = udiv i44 %244, %93
  %246 = zext i13 %73 to i29
  %247 = select i1 %195, i29 %246, i29 -1
  %248 = sext i9 %31 to i44
  %249 = sext i40 %168 to i44
  %maskA32 = trunc i44 %249 to i6
  %maskB33 = zext i6 %maskA32 to i44
  %250 = shl i44 %248, %maskB33
  %251 = zext i21 %129 to i58
  %252 = call i58 @llvm.cttz.i58(i58 %251, i1 false)
  %253 = freeze i1 %24
  %254 = sext i1 %253 to i51
  %255 = sext i13 %73 to i51
  %maskA34 = trunc i51 %255 to i6
  %256 = freeze i6 %maskA34
  %maskB35 = zext i6 %256 to i51
  %257 = add nsw i51 %254, %255
  %258 = sext i1 %229 to i44
  %259 = zext i1 %205 to i44
  %260 = call i44 @llvm.ssub.sat.i44(i44 %258, i44 %259)
  %261 = freeze i63 %207
  %262 = trunc i63 %261 to i44
  %263 = call i44 @llvm.usub.sat.i44(i44 %211, i44 %262)
  %264 = zext i1 %91 to i44
  %265 = trunc i54 %101 to i44
  %266 = icmp uge i44 %264, %265
  %267 = trunc i58 %235 to i44
  %268 = sext i13 %73 to i44
  %maskC36 = and i44 %268, 63
  %269 = sdiv exact i44 %267, %268
  %270 = zext i44 %157 to i63
  %271 = zext i44 %64 to i63
  %272 = zext i1 %226 to i63
  %273 = call i63 @llvm.fshr.i63(i63 %270, i63 %271, i63 %272)
  %274 = zext i34 %174 to i44
  %275 = icmp ugt i44 %274, %57
  %276 = zext i44 %243 to i59
  %277 = sext i1 %189 to i59
  %278 = trunc i10 %183 to i1
  %279 = select i1 %278, i59 %276, i59 %277
  %maskA37 = trunc i44 %17 to i6
  %maskB38 = sext i6 %maskA37 to i44
  %280 = add nuw i44 %177, %17
  %281 = zext i1 %136 to i44
  %282 = icmp ult i44 %214, %281
  %283 = trunc i63 %273 to i34
  %284 = trunc i44 %260 to i34
  %285 = call { i34, i1 } @llvm.sadd.with.overflow.i34(i34 %283, i34 %284)
  %286 = extractvalue { i34, i1 } %285, 1
  %287 = extractvalue { i34, i1 } %285, 0
  %288 = sext i44 %177 to i48
  %289 = zext i1 %286 to i48
  %290 = trunc i44 %57 to i1
  %291 = select i1 %290, i48 %288, i48 %289
  %292 = zext i1 %286 to i56
  ret i56 %292
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare { i38, i1 } @llvm.ssub.with.overflow.i38(i38, i38) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i19 @llvm.ssub.sat.i19(i19, i19) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i43 @llvm.cttz.i43(i43, i1 immarg) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i44 @llvm.fshr.i44(i44, i44, i44) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare { i6, i1 } @llvm.usub.with.overflow.i6(i6, i6) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i44 @llvm.fshl.i44(i44, i44, i44) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i45 @llvm.fshr.i45(i45, i45, i45) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i44 @llvm.smax.i44(i44, i44) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare { i40, i1 } @llvm.sadd.with.overflow.i40(i40, i40) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i44 @llvm.smin.i44(i44, i44) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i41 @llvm.smax.i41(i41, i41) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i63 @llvm.bitreverse.i63(i63) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i58 @llvm.cttz.i58(i58, i1 immarg) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i44 @llvm.ssub.sat.i44(i44, i44) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i44 @llvm.usub.sat.i44(i44, i44) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i63 @llvm.fshr.i63(i63, i63, i63) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare { i34, i1 } @llvm.sadd.with.overflow.i34(i34, i34) #0

attributes #0 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }
