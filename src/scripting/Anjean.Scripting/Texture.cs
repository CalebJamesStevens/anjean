namespace Anjean;

public sealed class Texture
{
    public string Filename { get; }
    public uint Width { get; }
    public uint Height { get; }
    public uint Channels { get; }

    public Texture(
        string filename,
        uint width = 0,
        uint height = 0,
        uint channels = 0
    )
    {
        Filename = filename;
        Width = width;
        Height = height;
        Channels = channels;
    }
}