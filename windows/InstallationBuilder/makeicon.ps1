param(
[string]$bit,
[string]$version
)
$path = 'C:\Program Files (x86)'
if($bit -eq '64') {$path = 'C:\Program Files'}

$shortcut = "$path\Bibledit-$version\Bibledit Desktop $version.lnk"
$target = "$path\Bibledit-$version\editor\bin\bibledit-desktop.exe"
$icon = "$path\Bibledit-$version\editor\bin\bibledit.ico"

$ws = New-Object -ComObject WScript.Shell
$s = $ws.CreateShortcut($shortcut)
$S.TargetPath = $target
$S.iconLocation = $icon
$S.Save()
