define <16 x i8> @reassociate_smax_v16i8(<16 x i8> %0, <16 x i8> %1, <16 x i8> %2, <16 x i8> %3) {
  %5 = add <16 x i8> %0, %1
  %6 = icmp sgt <16 x i8> %2, %5
  %7 = select <16 x i1> %6, <16 x i8> %2, <16 x i8> %5
  %8 = icmp sgt <16 x i8> %3, %7
  %9 = select <16 x i1> %8, <16 x i8> %3, <16 x i8> %7
  ret <16 x i8> %9
}
