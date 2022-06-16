using System.Runtime.CompilerServices;

[assembly: InternalsVisibleTo("Unity.WebRTC.RuntimeTests")]
[assembly: InternalsVisibleTo("Unity.WebRTC.EditorTests")]

// [Note: 2022/06/15] Allow external libraries to access internal members.
[assembly: InternalsVisibleTo("Unity.WebRTC.UtilityToolkit")]
