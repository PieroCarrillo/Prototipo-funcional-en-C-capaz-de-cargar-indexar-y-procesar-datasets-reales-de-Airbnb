param(
    [Parameter(Mandatory = $true)]
    [string]$InputPath,

    [Parameter(Mandatory = $true)]
    [string]$OutputPath,

    [int]$MaxDataRows = 100000,

    [string]$RequiredToken = ""
)

$inputStream = $null
$gzipStream = $null
$reader = $null
$writer = $null

try {
    $inputStream = [System.IO.File]::OpenRead($InputPath)
    $gzipStream = [System.IO.Compression.GZipStream]::new(
        $inputStream,
        [System.IO.Compression.CompressionMode]::Decompress
    )
    $reader = [System.IO.StreamReader]::new(
        $gzipStream,
        [System.Text.Encoding]::UTF8
    )
    $writer = [System.IO.StreamWriter]::new(
        $OutputPath,
        $false,
        [System.Text.UTF8Encoding]::new($false)
    )

    $header = $reader.ReadLine()
    if ($null -eq $header) {
        throw "El archivo comprimido no contiene una cabecera CSV."
    }
    $writer.WriteLine($header)

    $rows = 0
    $scanned = 0
    while ($rows -lt $MaxDataRows) {
        $line = $reader.ReadLine()
        if ($null -eq $line) {
            break
        }
        $scanned++
        if ($RequiredToken -ne "" -and -not $line.Contains($RequiredToken)) {
            continue
        }
        $writer.WriteLine($line)
        $rows++
    }

    Write-Host "Muestra creada: $rows filas seleccionadas de $scanned revisadas en $OutputPath"
}
finally {
    if ($null -ne $writer) { $writer.Dispose() }
    if ($null -ne $reader) { $reader.Dispose() }
    if ($null -ne $gzipStream) { $gzipStream.Dispose() }
    if ($null -ne $inputStream) { $inputStream.Dispose() }
}
