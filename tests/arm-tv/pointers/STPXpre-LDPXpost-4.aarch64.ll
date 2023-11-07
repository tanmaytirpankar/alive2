; ModuleID = 'M1'
source_filename = "M1"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-linux-gnu"

define i20 @f(i49 signext %0, i27 zeroext %1, i20 %2, i2 %3, i27 %4) {
  %6 = trunc i27 %4 to i20
  %7 = call i20 @llvm.cttz.i20(i20 %6, i1 true)
  %8 = call i30 @llvm.cttz.i30(i30 0, i1 false)
  %9 = trunc i20 %2 to i6
  %10 = trunc i49 %0 to i6
  %11 = icmp eq i6 %9, %10
  %12 = sext i2 %3 to i39
  %13 = zext i30 %8 to i39
  %14 = icmp sle i39 %12, %13
  %15 = zext i20 %2 to i62
  %16 = call i62 @llvm.abs.i62(i62 %15, i1 false)
  %17 = trunc i49 %0 to i34
  %18 = sext i20 %2 to i34
  %19 = sext i1 %14 to i34
  %20 = call i34 @llvm.fshr.i34(i34 %17, i34 %18, i34 %19)
  %21 = trunc i49 %0 to i24
  %22 = trunc i27 %4 to i24
  %23 = trunc i62 %16 to i24
  %24 = call i24 @llvm.fshl.i24(i24 %21, i24 %22, i24 %23)
  %25 = freeze i27 %1
  %26 = trunc i27 %25 to i24
  %27 = trunc i62 %16 to i24
  %28 = icmp ugt i24 %26, %27
  %29 = trunc i49 %0 to i34
  %30 = zext i1 %11 to i34
  %31 = call i34 @llvm.fshr.i34(i34 %29, i34 -134217728, i34 %30)
  %32 = trunc i24 %24 to i8
  %33 = trunc i27 %4 to i8
  %34 = icmp ugt i8 %32, %33
  %35 = zext i1 %34 to i63
  %36 = freeze i1 %34
  %37 = zext i1 %36 to i63
  %38 = trunc i20 %7 to i1
  %39 = select i1 %38, i63 %35, i63 %37
  %40 = trunc i34 %31 to i31
  %41 = call i31 @llvm.cttz.i31(i31 %40, i1 false)
  %42 = sext i1 %34 to i46
  %43 = trunc i63 %39 to i46
  %maskC = and i46 %43, 63
  %44 = sub nsw i46 %42, %43
  %45 = sext i1 %34 to i11
  %46 = freeze i49 %0
  %47 = trunc i49 %46 to i11
  %maskC1 = and i11 %47, 15
  %48 = srem i11 %45, %47
  %49 = zext i1 %28 to i5
  %50 = trunc i34 %31 to i5
  %51 = icmp ult i5 %49, %50
  %52 = freeze i31 %41
  %53 = zext i31 %52 to i32
  %54 = sext i30 %8 to i32
  %maskA = trunc i32 %54 to i5
  %maskB = zext i5 %maskA to i32
  %55 = sub nsw i32 %53, %54
  %56 = zext i2 %3 to i4
  %57 = trunc i11 %48 to i4
  %58 = icmp sle i4 %56, %57
  %59 = trunc i24 %24 to i2
  %60 = trunc i27 %1 to i2
  %61 = select i1 %28, i2 %59, i2 %60
  %62 = zext i24 %24 to i49
  %63 = sext i20 %7 to i49
  %64 = call i49 @llvm.uadd.sat.i49(i49 %62, i49 %63)
  %65 = trunc i62 %16 to i21
  %66 = trunc i62 %16 to i21
  %maskA2 = trunc i21 %66 to i5
  %maskB3 = zext i5 %maskA2 to i21
  %67 = ashr exact i21 %65, %maskB3
  %68 = sext i1 %14 to i49
  %69 = zext i1 %11 to i49
  %70 = call i49 @llvm.sadd.sat.i49(i49 %68, i49 %69)
  %71 = sext i1 %28 to i27
  %72 = sext i1 %11 to i27
  %73 = trunc i30 %8 to i1
  %74 = select i1 %73, i27 %71, i27 %72
  %75 = trunc i49 %70 to i9
  %76 = freeze i49 %70
  %77 = trunc i49 %76 to i9
  %78 = icmp ule i9 %75, %77
  %79 = trunc i63 %39 to i31
  %80 = sext i2 %3 to i31
  %81 = call i31 @llvm.sshl.sat.i31(i31 %79, i31 %80)
  %82 = trunc i49 %70 to i17
  %83 = trunc i27 %74 to i17
  %84 = trunc i34 %20 to i1
  %85 = select i1 %84, i17 %82, i17 %83
  %86 = sext i24 %24 to i37
  %87 = select i1 %14, i37 %86, i37 -52968
  %88 = trunc i46 %44 to i6
  %89 = trunc i20 %7 to i6
  %90 = zext i1 %58 to i6
  %91 = call i6 @llvm.fshr.i6(i6 %88, i6 %89, i6 %90)
  %92 = zext i11 %48 to i28
  %93 = sext i11 %48 to i28
  %maskC4 = and i28 %93, 31
  %94 = shl i28 %92, %93
  %95 = freeze i46 %44
  %96 = zext i46 %95 to i51
  %97 = sext i28 %94 to i51
  %maskA5 = trunc i51 %97 to i6
  %maskB6 = sext i6 %maskA5 to i51
  %98 = shl i51 %96, %maskB6
  %99 = zext i34 %31 to i43
  %100 = sext i1 %58 to i43
  %101 = icmp ult i43 %99, %100
  %102 = sext i1 %11 to i55
  %103 = sext i34 %20 to i55
  %104 = icmp eq i55 %102, %103
  %105 = sext i27 %1 to i38
  %106 = sext i37 %87 to i38
  %107 = trunc i17 %85 to i1
  %108 = select i1 %107, i38 %105, i38 %106
  %109 = zext i20 %7 to i39
  %110 = sext i38 %108 to i39
  %maskC7 = and i39 %110, 63
  %111 = xor i39 %109, %110
  %112 = trunc i17 %85 to i9
  %113 = freeze i20 %2
  %114 = trunc i20 %113 to i9
  %115 = trunc i31 %41 to i9
  %116 = call i9 @llvm.fshr.i9(i9 %112, i9 %114, i9 %115)
  %117 = freeze i27 %74
  %118 = zext i27 %117 to i29
  %119 = freeze i51 %98
  %120 = trunc i51 %119 to i29
  %121 = zext i2 %3 to i29
  %122 = call i29 @llvm.fshr.i29(i29 %118, i29 %120, i29 %121)
  %123 = zext i2 %61 to i53
  %124 = zext i21 %67 to i53
  %125 = select i1 %51, i53 %123, i53 %124
  %126 = trunc i34 %31 to i27
  %127 = icmp ugt i27 %126, %74
  %128 = trunc i27 %74 to i15
  %129 = trunc i34 %20 to i15
  %130 = trunc i2 %61 to i1
  %131 = select i1 %130, i15 %128, i15 %129
  %132 = sext i1 %58 to i44
  %133 = sext i1 %34 to i44
  %maskC8 = and i44 %133, 63
  %134 = shl nuw nsw i44 %132, %maskC8
  %135 = trunc i38 %108 to i18
  %136 = trunc i21 %67 to i18
  %137 = trunc i21 %67 to i1
  %138 = select i1 %137, i18 %135, i18 %136
  %139 = zext i1 %58 to i62
  %maskA9 = trunc i62 %139 to i6
  %maskB10 = zext i6 %maskA9 to i62
  %140 = or i62 -8791798054913, %139
  %141 = sext i34 %20 to i47
  %142 = call i47 @llvm.abs.i47(i47 %141, i1 true)
  %143 = freeze i44 %134
  %144 = trunc i44 %143 to i7
  %145 = trunc i31 %41 to i7
  %maskC11 = and i7 %145, 7
  %146 = udiv i7 %144, %145
  %147 = zext i17 %85 to i64
  %148 = sext i1 %14 to i64
  %maskC12 = and i64 %148, 63
  %149 = mul nuw i64 %147, %148
  %150 = sext i31 %81 to i34
  %151 = trunc i53 %125 to i34
  %152 = icmp sge i34 %150, %151
  %153 = zext i1 %28 to i21
  %154 = trunc i30 %8 to i21
  %155 = icmp sge i21 %153, %154
  %156 = sext i1 %28 to i52
  %157 = zext i2 %61 to i52
  %158 = sext i1 %11 to i52
  %159 = call i52 @llvm.fshl.i52(i52 %156, i52 %157, i52 %158)
  %160 = sext i1 %104 to i37
  %161 = trunc i52 %159 to i37
  %162 = select i1 %152, i37 %160, i37 %161
  %163 = sext i1 %11 to i44
  %164 = zext i1 %155 to i44
  %maskA13 = trunc i44 %164 to i6
  %maskB14 = zext i6 %maskA13 to i44
  %165 = sdiv exact i44 %163, %164
  %166 = zext i27 %74 to i58
  %167 = zext i37 %162 to i58
  %168 = trunc i38 %108 to i1
  %169 = select i1 %168, i58 %166, i58 %167
  %170 = sext i24 %24 to i51
  %171 = zext i49 %70 to i51
  %172 = trunc i20 %7 to i1
  %173 = select i1 %172, i51 %170, i51 %171
  %174 = sext i34 %20 to i45
  %175 = freeze i1 %34
  %176 = sext i1 %175 to i45
  %maskC15 = and i45 %176, 63
  %177 = lshr i45 %174, %176
  %178 = zext i15 %131 to i18
  %179 = trunc i39 %111 to i18
  %180 = trunc i37 %162 to i18
  %181 = call i18 @llvm.fshr.i18(i18 %178, i18 %179, i18 %180)
  %182 = trunc i32 %55 to i11
  %183 = trunc i37 %87 to i11
  %184 = trunc i24 %24 to i11
  %185 = call i11 @llvm.fshr.i11(i11 %182, i11 %183, i11 %184)
  %186 = zext i1 %155 to i47
  %187 = sext i1 %14 to i47
  %188 = icmp ne i47 %186, %187
  %189 = sext i24 %24 to i30
  %190 = call i30 @llvm.abs.i30(i30 %189, i1 false)
  %191 = zext i47 %142 to i50
  %192 = zext i31 %81 to i50
  %193 = sext i38 %108 to i50
  %194 = call i50 @llvm.fshl.i50(i50 %191, i50 %192, i50 %193)
  %195 = trunc i31 %81 to i28
  %196 = zext i17 %85 to i28
  %197 = icmp ult i28 %195, %196
  %198 = trunc i50 %194 to i41
  %199 = sext i1 %78 to i41
  %maskA16 = trunc i41 %199 to i6
  %maskB17 = zext i6 %maskA16 to i41
  %200 = sub nsw i41 %198, %199
  %201 = zext i27 %1 to i29
  %202 = zext i1 %152 to i29
  %203 = call i29 @llvm.usub.sat.i29(i29 %201, i29 %202)
  %204 = trunc i24 %24 to i18
  %205 = trunc i34 %20 to i18
  %206 = trunc i18 %138 to i1
  %207 = select i1 %206, i18 %204, i18 %205
  %208 = zext i1 %197 to i56
  %209 = zext i50 %194 to i56
  %210 = icmp slt i56 %208, %209
  %211 = freeze i1 %104
  %212 = zext i1 %211 to i38
  %213 = sext i21 %67 to i38
  %maskC18 = and i38 %213, 63
  %214 = sdiv exact i38 %212, %213
  %215 = trunc i9 %116 to i8
  %216 = trunc i63 %39 to i8
  %maskC19 = and i8 %216, 7
  %217 = or i8 %215, %216
  %218 = sext i15 %131 to i24
  %219 = zext i1 %188 to i24
  %maskC20 = and i24 %219, 31
  %220 = sdiv exact i24 %218, %219
  %221 = zext i24 %220 to i39
  %222 = zext i1 %28 to i39
  %maskC21 = and i39 %222, 63
  %223 = ashr exact i39 %221, %maskC21
  %224 = zext i8 %217 to i46
  %225 = sext i30 %8 to i46
  %226 = icmp sge i46 %224, %225
  %227 = freeze i28 %94
  %228 = trunc i28 %227 to i16
  %229 = trunc i46 %44 to i16
  %230 = icmp ugt i16 %228, %229
  %231 = sext i6 %91 to i31
  %232 = trunc i49 %0 to i31
  %233 = icmp ugt i31 %231, %232
  %234 = sext i1 %188 to i25
  %235 = zext i20 %2 to i25
  %236 = icmp ugt i25 %234, %235
  %237 = sext i30 %190 to i62
  %238 = zext i2 %3 to i62
  %239 = icmp sgt i62 %237, %238
  %240 = sext i9 %116 to i64
  %241 = zext i1 %230 to i64
  %242 = trunc i31 %81 to i1
  %243 = select i1 %242, i64 %240, i64 %241
  %244 = zext i1 %11 to i6
  %245 = trunc i29 %122 to i6
  %maskA22 = trunc i6 %245 to i3
  %maskB23 = sext i3 %maskA22 to i6
  %246 = ashr i6 %244, %maskB23
  %247 = trunc i46 %44 to i9
  %248 = trunc i58 %169 to i9
  %249 = trunc i39 %223 to i1
  %250 = select i1 %249, i9 %247, i9 %248
  %251 = zext i1 %239 to i5
  %252 = call i5 @llvm.cttz.i5(i5 %251, i1 true)
  %253 = sext i11 %185 to i30
  %254 = zext i1 %101 to i30
  %maskA24 = trunc i30 %254 to i5
  %maskB25 = sext i5 %maskA24 to i30
  %255 = ashr i30 %253, %maskB25
  %256 = trunc i38 %214 to i6
  %257 = trunc i29 %203 to i6
  %258 = icmp sge i6 %256, %257
  %259 = sext i49 %64 to i55
  %260 = zext i2 %3 to i55
  %261 = zext i1 %34 to i55
  %262 = call i55 @llvm.fshl.i55(i55 %259, i55 %260, i55 %261)
  %263 = zext i1 %210 to i49
  %264 = zext i41 %200 to i49
  %265 = call i49 @llvm.usub.sat.i49(i49 %263, i49 %264)
  %266 = trunc i32 %55 to i22
  %267 = icmp eq i22 %266, -15
  %268 = sext i39 %111 to i51
  %269 = call i51 @llvm.abs.i51(i51 %268, i1 true)
  %270 = sext i1 %267 to i27
  %271 = zext i9 %116 to i27
  %272 = trunc i64 %243 to i1
  %273 = select i1 %272, i27 %270, i27 %271
  %274 = zext i1 %152 to i17
  %275 = sext i1 %233 to i17
  %maskA26 = trunc i17 %275 to i5
  %maskB27 = zext i5 %maskA26 to i17
  %276 = urem i17 %274, %275
  %277 = trunc i49 %70 to i20
  %278 = trunc i37 %87 to i20
  %279 = select i1 %233, i20 %277, i20 %278
  %280 = trunc i49 %70 to i43
  %281 = sext i37 %87 to i43
  %282 = trunc i64 %149 to i1
  %283 = select i1 %282, i43 %280, i43 %281
  %284 = sext i31 %41 to i63
  %285 = sext i51 %269 to i63
  %286 = trunc i38 %214 to i1
  %287 = select i1 %286, i63 %284, i63 %285
  %288 = trunc i47 %142 to i40
  %289 = zext i27 %4 to i40
  %290 = call { i40, i1 } @llvm.sadd.with.overflow.i40(i40 %288, i40 %289)
  %291 = extractvalue { i40, i1 } %290, 1
  %292 = extractvalue { i40, i1 } %290, 0
  %293 = zext i1 %14 to i42
  %294 = trunc i62 %16 to i42
  %295 = trunc i28 %94 to i1
  %296 = select i1 %295, i42 %293, i42 %294
  %297 = sext i27 %273 to i35
  %298 = zext i27 %4 to i35
  %299 = icmp sle i35 %297, %298
  %300 = sext i27 %1 to i60
  %301 = zext i1 %152 to i60
  %302 = sext i1 %236 to i60
  %303 = call i60 @llvm.fshl.i60(i60 %300, i60 %301, i60 %302)
  %304 = sext i18 %181 to i20
  ret i20 %304
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i20 @llvm.cttz.i20(i20, i1 immarg) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i30 @llvm.cttz.i30(i30, i1 immarg) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i62 @llvm.abs.i62(i62, i1 immarg) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i34 @llvm.fshr.i34(i34, i34, i34) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i24 @llvm.fshl.i24(i24, i24, i24) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i31 @llvm.cttz.i31(i31, i1 immarg) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i49 @llvm.uadd.sat.i49(i49, i49) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i49 @llvm.sadd.sat.i49(i49, i49) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i31 @llvm.sshl.sat.i31(i31, i31) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i6 @llvm.fshr.i6(i6, i6, i6) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i9 @llvm.fshr.i9(i9, i9, i9) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i29 @llvm.fshr.i29(i29, i29, i29) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i47 @llvm.abs.i47(i47, i1 immarg) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i52 @llvm.fshl.i52(i52, i52, i52) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i18 @llvm.fshr.i18(i18, i18, i18) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i11 @llvm.fshr.i11(i11, i11, i11) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i30 @llvm.abs.i30(i30, i1 immarg) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i50 @llvm.fshl.i50(i50, i50, i50) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i29 @llvm.usub.sat.i29(i29, i29) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i5 @llvm.cttz.i5(i5, i1 immarg) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i55 @llvm.fshl.i55(i55, i55, i55) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i49 @llvm.usub.sat.i49(i49, i49) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i51 @llvm.abs.i51(i51, i1 immarg) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare { i40, i1 } @llvm.sadd.with.overflow.i40(i40, i40) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i60 @llvm.fshl.i60(i60, i60, i60) #0

attributes #0 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }
