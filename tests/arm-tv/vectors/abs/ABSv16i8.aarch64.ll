define <16 x i8> @f5(<16 x i8> %0) {
  %2 = icmp slt <16 x i8> %0, zeroinitializer
  %3 = sub <16 x i8> zeroinitializer, %0
  %4 = select <16 x i1> %2, <16 x i8> %3, <16 x i8> %0
  %5 = sub <16 x i8> zeroinitializer, %4
  ret <16 x i8> %5
}
