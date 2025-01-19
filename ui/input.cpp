Buffer
ReadFile(Arena* arena, const std::string& filename)
{
    Buffer buffer = {0};
    FILE* file = fopen(filename.data(), "rb");
    if (file == NULL)
    {
        exitWithError("failed to open file!");
    }

    fseek(file, 0, SEEK_END);
    buffer.size = (u64)ftell(file);
    fseek(file, 0, SEEK_SET);

    buffer.data = PushArray(arena, u8, buffer.size);
    fread(buffer.data, sizeof(u8), buffer.size, file);

    fclose(file);
    return buffer;
}