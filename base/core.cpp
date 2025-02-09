template <typename N>
N
Clamp(N d, N min, N max)
{
    N temp = d > min ? d : min;
    return temp < max ? temp : max;
}