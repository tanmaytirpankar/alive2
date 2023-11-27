define <2 x i32> @negated_operand_commute_vec(<2 x i32> %0) {
  %2 = sub <2 x i32> zeroinitializer, %0
  %3 = srem <2 x i32> %2, %0
  ret <2 x i32> %3
}
