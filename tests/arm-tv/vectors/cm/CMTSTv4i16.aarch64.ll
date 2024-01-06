define <4 x i16> @f(ptr %0, ptr %1) {
  %3 = load <4 x i16>, ptr %0, align 8
  %4 = load <4 x i16>, ptr %1, align 8
  %5 = and <4 x i16> %3, %4
  %6 = icmp ne <4 x i16> %5, zeroinitializer
  %7 = sext <4 x i1> %6 to <4 x i16>
  ret <4 x i16> %7
}
