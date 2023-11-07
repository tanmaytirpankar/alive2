; ModuleID = 'M1'
source_filename = "M1"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-linux-gnu"

define i46 @f(i16 signext %0, i8 zeroext %1, i46 noundef zeroext %2) {
  %4 = zext i16 %0 to i64
  %5 = zext i16 %0 to i64
  %maskC = and i64 %5, 63
  %6 = lshr i64 %4, %5
  %7 = trunc i8 %1 to i1
  %8 = trunc i46 %2 to i1
  %maskC1 = and i1 %8, true
  %9 = shl nuw i1 %7, %maskC1
  %10 = trunc i64 %6 to i32
  %11 = sext i16 %0 to i32
  %12 = icmp eq i32 %10, %11
  %13 = zext i1 %12 to i16
  %14 = sext i1 %12 to i16
  %maskC2 = and i16 %14, 15
  %15 = sub i16 %13, %14
  %16 = sext i8 %1 to i46
  %17 = freeze i16 %15
  %18 = sext i16 %17 to i46
  %maskC3 = and i46 %18, 63
  %19 = sdiv exact i46 %16, %18
  %20 = zext i16 %0 to i46
  %maskA = trunc i46 %19 to i6
  %maskB = sext i6 %maskA to i46
  %21 = udiv i46 %20, %19
  %22 = zext i16 %0 to i46
  %23 = icmp uge i46 %22, %19
  %24 = trunc i64 %6 to i46
  %25 = call i46 @llvm.abs.i46(i46 %24, i1 false)
  %26 = freeze i64 %6
  %27 = trunc i64 %26 to i16
  %28 = trunc i46 %25 to i16
  %29 = icmp ne i16 %27, %28
  %maskC4 = and i8 %1, 7
  %30 = ashr exact i8 %1, %maskC4
  %31 = sext i1 %23 to i46
  %32 = sext i1 %23 to i46
  %33 = trunc i64 %6 to i1
  %34 = select i1 %33, i46 %31, i46 %32
  %35 = zext i8 %1 to i16
  %36 = zext i1 %29 to i16
  %37 = call i16 @llvm.usub.sat.i16(i16 %35, i16 %36)
  %38 = trunc i16 %15 to i8
  %39 = sext i1 %12 to i8
  %maskA5 = trunc i8 %39 to i3
  %maskB6 = sext i3 %maskA5 to i8
  %40 = and i8 %38, %39
  %41 = zext i8 %30 to i46
  %maskC7 = and i46 %41, 63
  %42 = udiv i46 %21, %41
  %43 = trunc i46 %21 to i16
  %44 = zext i8 %30 to i16
  %45 = icmp eq i16 %43, %44
  %46 = zext i16 %0 to i46
  %47 = sext i1 %12 to i46
  %48 = call i46 @llvm.fshl.i46(i46 %46, i46 %42, i46 %47)
  %49 = freeze i8 %30
  %50 = sext i8 %49 to i46
  %51 = icmp sge i46 %50, %48
  %52 = icmp eq i46 %2, %34
  %53 = zext i1 %45 to i64
  %54 = zext i16 %15 to i64
  %55 = trunc i16 %0 to i1
  %56 = select i1 %55, i64 %53, i64 %54
  %57 = trunc i16 %37 to i8
  %58 = sext i1 %9 to i8
  %59 = icmp ugt i8 %57, %58
  %60 = sext i1 %59 to i8
  %61 = call i8 @llvm.smax.i8(i8 %60, i8 %1)
  %62 = sext i1 %23 to i46
  %63 = freeze i16 %37
  %64 = sext i16 %63 to i46
  %maskC8 = and i46 %64, 63
  %65 = udiv i46 %62, %64
  %66 = trunc i46 %65 to i1
  %67 = trunc i8 %1 to i1
  %68 = select i1 %67, i1 %45, i1 %66
  %maskC9 = and i46 %48, 63
  %69 = urem i46 1, %48
  %70 = zext i46 %34 to i64
  %71 = sext i46 %48 to i64
  %72 = select i1 %12, i64 %70, i64 %71
  %73 = freeze i64 %72
  %74 = trunc i64 %73 to i1
  %75 = select i1 %74, i46 %34, i46 %34
  %76 = zext i8 %40 to i46
  %77 = icmp ugt i46 %2, %76
  %78 = sext i1 %77 to i46
  %79 = icmp ne i46 %78, %48
  %80 = trunc i64 %6 to i46
  %81 = sext i1 %68 to i46
  %maskC10 = and i46 %81, 63
  %82 = srem i46 %80, %81
  %83 = zext i1 %9 to i46
  %84 = call i46 @llvm.uadd.sat.i46(i46 %25, i46 %83)
  %85 = zext i1 %59 to i32
  %86 = freeze i16 %0
  %87 = sext i16 %86 to i32
  %88 = call i32 @llvm.smin.i32(i32 %85, i32 %87)
  %89 = trunc i46 %48 to i32
  %90 = trunc i46 %69 to i32
  %91 = trunc i46 %75 to i1
  %92 = select i1 %91, i32 %89, i32 %90
  %93 = sext i8 %1 to i46
  %94 = call i46 @llvm.ushl.sat.i46(i46 %93, i46 %19)
  %95 = freeze i1 %12
  %96 = sext i1 %95 to i46
  %97 = sext i1 %29 to i46
  %98 = zext i8 %30 to i46
  %99 = call i46 @llvm.fshl.i46(i46 %96, i46 %97, i46 %98)
  %100 = freeze i1 %59
  %101 = zext i1 %100 to i46
  %102 = zext i16 %0 to i46
  %103 = trunc i46 %84 to i1
  %104 = select i1 %103, i46 %101, i46 %102
  %105 = trunc i32 %92 to i8
  %106 = icmp uge i8 %105, 1
  %107 = trunc i8 %61 to i1
  %108 = select i1 %107, i46 %82, i46 %75
  %109 = sext i1 %68 to i46
  %110 = select i1 %59, i46 %94, i46 %109
  %111 = zext i1 %51 to i16
  %112 = freeze i46 %34
  %113 = trunc i46 %112 to i16
  %114 = call i16 @llvm.fshr.i16(i16 %111, i16 %0, i16 %113)
  %maskC11 = and i46 %104, 63
  %115 = lshr exact i46 %94, %104
  %116 = zext i16 %0 to i46
  %117 = icmp ult i46 %48, %116
  %118 = zext i8 %61 to i64
  %119 = sext i1 %51 to i64
  %120 = zext i32 %92 to i64
  %121 = call i64 @llvm.fshr.i64(i64 %118, i64 %119, i64 %120)
  %122 = sext i1 %29 to i46
  %123 = trunc i46 %84 to i1
  %124 = select i1 %123, i46 %34, i46 %122
  %125 = zext i8 %61 to i46
  %126 = icmp ult i46 %125, 29168
  %127 = zext i1 %68 to i8
  %128 = trunc i46 %84 to i8
  %129 = icmp eq i8 %127, %128
  %130 = freeze i46 %65
  %131 = sext i46 %130 to i64
  %132 = sext i1 %126 to i64
  %133 = select i1 %106, i64 %131, i64 %132
  %134 = zext i1 %29 to i46
  %135 = zext i1 %9 to i46
  %maskC12 = and i46 %135, 63
  %136 = mul nsw i46 %134, %135
  %137 = trunc i46 %21 to i16
  %138 = trunc i46 %82 to i16
  %139 = trunc i8 %40 to i1
  %140 = select i1 %139, i16 %137, i16 %138
  %141 = sext i32 %88 to i64
  %142 = sext i16 %114 to i64
  %143 = icmp sgt i64 %141, %142
  %144 = zext i1 %117 to i16
  %145 = icmp ule i16 %144, %0
  %146 = zext i1 %59 to i46
  %147 = zext i1 %51 to i46
  %maskA13 = trunc i46 %147 to i6
  %maskB14 = sext i6 %maskA13 to i46
  %148 = mul nuw i46 %146, %147
  %149 = trunc i46 %34 to i1
  %150 = ashr exact i1 %143, %149
  %151 = zext i46 %75 to i64
  %152 = sext i1 %12 to i64
  %153 = icmp ne i64 %151, %152
  %154 = zext i1 %153 to i64
  %155 = sext i46 %75 to i64
  %156 = call i64 @llvm.smax.i64(i64 %154, i64 %155)
  %157 = trunc i64 %72 to i1
  %158 = trunc i46 %34 to i1
  %159 = select i1 %158, i1 %157, i1 %45
  %160 = freeze i46 %21
  %161 = zext i32 %92 to i46
  %162 = freeze i46 %136
  %163 = trunc i46 %162 to i1
  %164 = select i1 %163, i46 %160, i46 %161
  %165 = zext i1 %150 to i16
  %166 = call i16 @llvm.bswap.i16(i16 %165)
  %167 = trunc i46 %136 to i8
  %168 = trunc i46 %34 to i8
  %169 = icmp ult i8 %167, %168
  %170 = freeze i46 %148
  %171 = sext i1 %129 to i46
  %maskC15 = and i46 %171, 63
  %172 = udiv i46 %170, %171
  %173 = freeze i46 %124
  %174 = freeze i1 %79
  %175 = select i1 %174, i46 %110, i46 %173
  %176 = sext i1 %23 to i64
  %177 = zext i1 %59 to i64
  %maskA16 = trunc i64 %177 to i6
  %maskB17 = sext i6 %maskA16 to i64
  %178 = udiv i64 %176, %177
  %179 = freeze i64 %178
  %180 = trunc i64 %179 to i46
  %181 = trunc i64 %56 to i46
  %182 = icmp uge i46 %180, %181
  %183 = trunc i64 %56 to i46
  %maskC18 = and i46 %183, 63
  %184 = xor i46 %110, %183
  %185 = zext i1 %129 to i8
  %186 = trunc i64 %133 to i8
  %187 = call { i8, i1 } @llvm.uadd.with.overflow.i8(i8 %185, i8 %186)
  %188 = extractvalue { i8, i1 } %187, 1
  %189 = extractvalue { i8, i1 } %187, 0
  %190 = sext i16 %140 to i64
  %maskC19 = and i64 %178, 63
  %191 = mul nsw i64 %190, %178
  %192 = sext i8 %189 to i46
  %193 = zext i1 %143 to i46
  %194 = freeze i32 %88
  %195 = sext i32 %194 to i46
  %196 = call i46 @llvm.fshr.i46(i46 %192, i46 %193, i46 %195)
  %197 = trunc i46 %172 to i8
  %198 = trunc i46 %42 to i8
  %199 = freeze i1 %150
  %200 = select i1 %199, i8 %197, i8 %198
  %201 = zext i1 %45 to i46
  %202 = sext i8 %61 to i46
  %203 = icmp ult i46 %201, %202
  %204 = sext i1 %143 to i46
  %205 = zext i1 %29 to i46
  %206 = trunc i46 %124 to i1
  %207 = select i1 %206, i46 %204, i46 %205
  %208 = trunc i46 %196 to i1
  %209 = trunc i46 %164 to i1
  %210 = select i1 %159, i1 %208, i1 %209
  %211 = trunc i46 %136 to i8
  %212 = trunc i64 %72 to i8
  %maskC20 = and i8 %212, 7
  %213 = and i8 %211, %212
  %214 = sext i1 %203 to i46
  %215 = zext i8 %30 to i46
  %maskC21 = and i46 %215, 63
  %216 = lshr exact i46 %214, %maskC21
  %217 = sext i1 %29 to i16
  %218 = trunc i46 %148 to i16
  %219 = icmp sge i16 %217, %218
  %220 = zext i46 %148 to i64
  %221 = sext i1 %153 to i64
  %maskC22 = and i64 %221, 63
  %222 = srem i64 %220, %221
  %223 = trunc i46 %2 to i1
  %224 = mul nuw i1 %223, %182
  %225 = trunc i16 %0 to i1
  %226 = trunc i64 %56 to i1
  %227 = call i1 @llvm.fshr.i1(i1 %225, i1 %159, i1 %226)
  %228 = freeze i46 %19
  %229 = trunc i46 %228 to i8
  %230 = trunc i46 %69 to i8
  %231 = call { i8, i1 } @llvm.sadd.with.overflow.i8(i8 %229, i8 %230)
  %232 = extractvalue { i8, i1 } %231, 1
  %233 = extractvalue { i8, i1 } %231, 0
  %234 = sext i1 %117 to i8
  %235 = freeze i1 %232
  %236 = zext i1 %235 to i8
  %237 = icmp ne i8 %234, %236
  %238 = zext i16 %15 to i46
  %239 = trunc i64 %222 to i46
  %240 = trunc i64 %121 to i1
  %241 = select i1 %240, i46 %238, i46 %239
  %242 = zext i1 %188 to i64
  %243 = zext i16 %0 to i64
  %244 = icmp slt i64 %242, %243
  %245 = trunc i46 %136 to i16
  %246 = trunc i64 %72 to i16
  %247 = select i1 %227, i16 %245, i16 %246
  %248 = sext i1 %227 to i46
  %249 = icmp ule i46 %248, %65
  %250 = zext i1 %68 to i46
  %251 = zext i1 %23 to i46
  %maskC23 = and i46 %251, 63
  %252 = srem i46 %250, %251
  %253 = trunc i46 %196 to i1
  %254 = call { i1, i1 } @llvm.uadd.with.overflow.i1(i1 %253, i1 %237)
  %255 = extractvalue { i1, i1 } %254, 1
  %256 = extractvalue { i1, i1 } %254, 0
  %257 = zext i1 %249 to i46
  %258 = icmp slt i46 %65, %257
  %259 = zext i1 %210 to i46
  %260 = sext i16 %15 to i46
  %261 = select i1 %258, i46 %259, i46 %260
  %maskA24 = trunc i46 %65 to i6
  %maskB25 = zext i6 %maskA24 to i46
  %262 = urem i46 %94, %65
  %263 = trunc i46 %84 to i1
  %maskC26 = and i1 %263, true
  %264 = sub i1 %150, %263
  %265 = zext i16 %166 to i64
  %266 = zext i46 %108 to i64
  %267 = freeze i46 %136
  %268 = trunc i46 %267 to i1
  %269 = select i1 %268, i64 %265, i64 %266
  %270 = trunc i46 %262 to i8
  %271 = trunc i46 %207 to i8
  %maskA27 = trunc i8 %271 to i3
  %maskB28 = zext i3 %maskA27 to i8
  %272 = mul nuw nsw i8 %270, %271
  %273 = sext i8 %30 to i46
  %274 = call i46 @llvm.bitreverse.i46(i46 %273)
  %275 = sext i1 %23 to i8
  %276 = sext i1 %77 to i8
  %277 = trunc i46 %48 to i1
  %278 = select i1 %277, i8 %275, i8 %276
  %279 = zext i1 %210 to i32
  %280 = sext i8 %1 to i32
  %281 = trunc i46 %104 to i1
  %282 = select i1 %281, i32 %279, i32 %280
  %283 = trunc i46 %261 to i16
  %284 = sext i8 %278 to i16
  %285 = icmp slt i16 %283, %284
  %286 = zext i1 %129 to i46
  %287 = zext i1 %59 to i46
  %288 = call i46 @llvm.fshr.i46(i46 %286, i46 %287, i46 %42)
  %289 = sext i16 %114 to i64
  %290 = freeze i1 %45
  %291 = zext i1 %290 to i64
  %maskC29 = and i64 %291, 63
  %292 = udiv i64 %289, %291
  %maskA30 = trunc i64 %133 to i6
  %maskB31 = sext i6 %maskA30 to i64
  %293 = or i64 %156, %133
  %294 = zext i46 %184 to i64
  %295 = call i64 @llvm.bitreverse.i64(i64 %294)
  %296 = zext i1 %285 to i46
  ret i46 %296
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i46 @llvm.abs.i46(i46, i1 immarg) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i16 @llvm.usub.sat.i16(i16, i16) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i46 @llvm.fshl.i46(i46, i46, i46) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i8 @llvm.smax.i8(i8, i8) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i46 @llvm.uadd.sat.i46(i46, i46) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i32 @llvm.smin.i32(i32, i32) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i46 @llvm.ushl.sat.i46(i46, i46) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i16 @llvm.fshr.i16(i16, i16, i16) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i64 @llvm.fshr.i64(i64, i64, i64) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i64 @llvm.smax.i64(i64, i64) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i16 @llvm.bswap.i16(i16) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare { i8, i1 } @llvm.uadd.with.overflow.i8(i8, i8) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i46 @llvm.fshr.i46(i46, i46, i46) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i1 @llvm.fshr.i1(i1, i1, i1) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare { i8, i1 } @llvm.sadd.with.overflow.i8(i8, i8) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare { i1, i1 } @llvm.uadd.with.overflow.i1(i1, i1) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i46 @llvm.bitreverse.i46(i46) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i64 @llvm.bitreverse.i64(i64) #0

attributes #0 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }
