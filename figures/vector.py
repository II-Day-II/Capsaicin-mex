
class vec2:
    def __init__(self, x, y = None):
        if y is None:
            self.x = x
            self.y = x
        else:
            self.x = x
            self.y = y

    def __add__(self, other):
        if isinstance(other, vec2):
            return vec2(self.x + other.x, self.y + other.y)
        else:
            return vec2(self.x + other, self.y + other)

    def __sub__(self, other):
        if isinstance(other, vec2):
            return vec2(self.x - other.x, self.y - other.y)
        else:
            return vec2(self.x - other, self.y - other)

    def __mul__(self, other):
        if isinstance(other, vec2):
            return vec2(self.x * other.x, self.y * other.y)
        else:
            return vec2(self.x * other, self.y * other)

    def __floordiv__(self, other):
        if isinstance(other, vec2):
            return vec2(self.x // other.x, self.y // other.y)
        else:
            return vec2(self.x // other, self.y // other)

    def __truediv__(self, other):
        if isinstance(other, vec2):
            return vec2(self.x / other.x, self.y / other.y)
        else:
            return vec2(self.x / other, self.y / other)

    def __str__(self):
        return f"vec2({self.x}, {self.y}))"

    def dot(self, other):
        return self.x * other.x + self.y * other.y

    def length(self):
        return math.sqrt(self.x ** 2 + self.y ** 2)

    def normalize(self):
        return self / self.length()

    def frac(self):
        x_i, x_f = math.modf(self.x)
        y_i, y_f = math.modf(self.y)
        return vec2(x_f, y_f)

class vec3(vec2):
    def __init__(self, x, y=None, z=None):
        super().__init__(x, y)
        if z is None:
            self.z = x
        else:
            self.z = z

    def length(self):
        return math.sqrt(self.x ** 2 + self.y ** 2 + self.z ** 2)
    
    def normalize(self):
        return self / self.length()

    def dot(self, other):
        return self.x * other.x + self.y * other.y + self.z * other.z

    def __truediv__(self, other):
        if isinstance(other, vec3):
            return vec3(self.x / other.x, self.y / other.y, self.z / other.z)
        else:
            return vec3(self.x / other, self.y / other, self.z / other)

    def __str__(self):
        return f"vec3({self.x}, {self.y}, {self.z})"

