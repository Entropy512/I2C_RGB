#    Purpose: Format a number into a different base.
#    Created: ????-??-?? By Nick Coghlan
#    Assuming Public Domain License as this code was found on a Python help forum

# val         - the value to convert
# base        - the base into which the conversion should occur
# min_digits  - minimum number of digits to print
# @return     - string representing val in base
def show_base(val, base, min_digits=1, complement=False,
                digits="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"):
        if base > len(digits): raise ValueError("Not enough digits for base")
        negative = val < 0
        val = abs(val)
        if complement:
            sign = ""
            max = base**min_digits
            if (val >= max) or (not negative and val == max):
                raise ValueError("Value out of range for complemented format")
            if negative:
                val = (max - val)
        else:
            sign = "-" * negative
        val_digits = []
        while val:
            val, digit = divmod(val, base)
            val_digits.append(digits[digit])
        result = "".join(reversed(val_digits))
        return sign + ("0" * (min_digits - len(result))) + result


if __name__ == '__main__':
    print "Testing show_base function"
    print show_base(0, 2), " = ", '0'
    print show_base(1, 2), " = ", '1'
    print show_base(127, 2), " = ", '1111111'
    print show_base(255, 2), " = ", '11111111'
    print show_base(10, 2), " = ", '1010'
    print show_base(-10, 2), " = ", '-1010'
    print show_base(10, 2, 8), " = ", '00001010'
    print show_base(-10, 2, 8), " = ", '-00001010'
    print show_base(10, 2, 8, complement=True), " = ", '00001010'
    print show_base(-10, 2, 8, complement=True), " = ", '11110110'
    print show_base(10, 16, 2, complement=True), " = ", '0A'
    print show_base(-10, 16, 2, complement=True), " = ", 'F6'
    print show_base(127, 16, 2, complement=True), " = ", '7F'
    print show_base(-127, 16, 2, complement=True), " = ", '81'
    print show_base(255, 16, 2, complement=True), " = ", 'FF'
    print show_base(-255, 16, 2, complement=True), " = ", '01'
