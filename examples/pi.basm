#
# π = (4/1) - (4/3) + (4/5) - (4/7) + (4/9) - (4/11) + (4/13) - (4/15) ...
# Take 4 and subtract 4 divided by 3. Then add 4 divided by 5.
# Then subtract 4 divided by 7. Continue alternating between adding
# and subtracting fractions with a numerator of 4 and a denominator of each
# subsequent odd number. The more times you do this, the closer you will get to pi.
#

push 4.0        # acc (result of first division 4/1)
push 3.0        # denominator
push 750000     # counter

loop:
    swap 2      # swap counter (top of stack) with current acc

    # calculate next denominator

    push 4.0
    dup 2
    push 2.0
    plusf
    swap 3

    # calculate with current denominator

    divf        #        (4/n)
    minusf      # acc - ^

    # calculate next denominator

    push 4.0
    dup 2
    push 2.0
    plusf
    swap 3

    # calculate with current denominator

    divf        #        (4/n)
    plusf       # acc + ^

    # decrement counter
    swap 2
    push 1
    minusi

    dup 0       # duplicate current counter since jmp_if consumes top of stack
    jmp_if loop

# clean the stack and only have pi left
drop
drop

halt
