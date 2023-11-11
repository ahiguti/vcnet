
import numpy

m = [[77, 150, 29], [-43, -84, 128], [127, -106, -21]]
print(m)
for i in range(3):
  for j in range(3):
    m[i][j] = m[i][j] / 255.0
print(m)
mi = numpy.linalg.inv(m)
print(mi)
for i in range(3):
  for j in range(3):
    mi[i][j] = mi[i][j] * 255.0
print(mi)
for i in range(3):
  for j in range(3):
    mi[i][j] = round(mi[i][j])
print(mi)



