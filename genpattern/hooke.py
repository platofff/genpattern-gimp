import copy


class Continue(BaseException):
    pass


def hooke(f, b1, step, accuracy, target=float('-inf')):
    steps = [step] * len(b1)

    def first_phase(b1, change_steps=True):
        r1 = f(*b1)
        b2 = copy.copy(b1)
        r2 = r1
        while all(b1[i] == b2[i] for i in range(len(b1))):
            for i in range(len(b1)):
                if steps[i] < accuracy:
                    continue
                b = copy.copy(b2)
                b[i] += steps[i]
                forward = f(*b)
                b[i] = b2[i] - steps[i]
                backward = f(*b)
                if r1 <= forward and r1 <= backward:
                    if change_steps:
                        steps[i] //= 10 # change to /= 10 for floats
                elif backward < forward:
                    b2[i] = b2[i] - steps[i]
                    r2 = backward
                else:
                    b2[i] = b2[i] + steps[i]
                    r2 = forward
            if not change_steps and r1 == r2:
                return b2, False
            if all(steps[i] <= accuracy for i in range(len(steps))):
                return b2, True

        return b2, False

    while True:
        b2, stop = first_phase(b1)
        res2 = f(*b2)
        if stop or res2 <= target:
            return b2, res2
        while True:
            b3 = [b1[i] + 2 * (b2[i] - b1[i]) for i in range(len(b1))]
            b4, _ = first_phase(b3, False)
            try:
                res4 = f(*b4)
                if f(*b2) > res4:
                    if res4 <= target:
                        return b4, res4
                    b1 = b2
                    b2 = b4
                    raise Continue
                else:
                    b1 = b2
            except Continue:
                continue
            break
