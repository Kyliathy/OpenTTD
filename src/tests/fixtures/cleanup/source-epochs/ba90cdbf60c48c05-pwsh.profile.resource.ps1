## Copyright (c) Microsoft Corporation. All rights reserved.
## Licensed under the MIT License.

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('get', 'set', 'export')]
    [string]$Operation,
    [Parameter(ValueFromPipeline)]
    [string[]]$UserInput
)

Begin {
    enum ProfileType {
        AllUsersCurrentHost
        AllUsersAllHosts
        CurrentUserAllHosts
        CurrentUserCurrentHost
    }

    function New-PwshResource {
        param(
            [Parameter(Mandatory = $true)]
            [ProfileType] $ProfileType,

            [Parameter(ParameterSetName = 'WithContent')]
            [string] $Content,

            [Parameter(ParameterSetName = 'WithContent')]
            [bool] $Exist
        )

        # Create the PSCustomObject with properties
        $resource = [PSCustomObject]@{
            profileType = $ProfileType
            content     = $null
            profilePath = GetProfilePath -profileType $ProfileType
            _exist      = $false
        }

        # Add ToJson method
        $resource | Add-Member -MemberType ScriptMethod -Name 'ToJson' -Value {
            return ([ordered] @{
                    profileType = $this.profileType
                    content     = $this.content
                    profilePath = $this.profilePath
                    _exist      = $this._exist
                }) | ConvertTo-Json -Compress -EnumsAsStrings
        }

        # Constructor logic - if Content and Exist parameters are provided (WithContent parameter set)
        if ($PSCmdlet.ParameterSetName -eq 'WithContent') {
            $resource.content = $Content
            $resource._exist = $Exist
        } else {
            # Default constructor logic - read from file system
            $fileExists = Test-Path $resource.profilePath
            if ($fileExists) {
                $resource.content = Get-Content -Path $resource.profilePath
            } else {
                $resource.content = $null
            }
            $resource._exist = $fileExists
        }

        return $resource
    }

    function GetProfilePath {
        param (
            [ProfileType] $profileType
        )

        $path = switch ($profileType) {
            'AllUsersCurrentHost' { $PROFILE.AllUsersCurrentHost }
            'AllUsersAllHosts' { $PROFILE.AllUsersAllHosts }
            'CurrentUserAllHosts' { $PROFILE.CurrentUserAllHosts }
            'CurrentUserCurrentHost' { $PROFILE.CurrentUserCurrentHost }
        }

        return $path
    }

    function ExportOperation {
        $allUserCurrentHost = New-PwshResource -ProfileType 'AllUsersCurrentHost'
        $allUsersAllHost = New-PwshResource -ProfileType 'AllUsersAllHosts'
        $currentUserAllHost = New-PwshResource -ProfileType 'CurrentUserAllHosts'
        $currentUserCurrentHost = New-PwshResource -ProfileType 'CurrentUserCurrentHost'

        # Cannot use the ToJson() method here as we are adding a note property
        $allUserCurrentHost | Add-Member -NotePropertyName '_name' -NotePropertyValue 'AllUsersCurrentHost' -PassThru | ConvertTo-Json -Compress -EnumsAsStrings
        $allUsersAllHost | Add-Member -NotePropertyName '_name' -NotePropertyValue 'AllUsersAllHosts' -PassThru | ConvertTo-Json -Compress -EnumsAsStrings
        $currentUserAllHost | Add-Member -NotePropertyName '_name' -NotePropertyValue 'CurrentUserAllHosts' -PassThru | ConvertTo-Json -Compress -EnumsAsStrings
        $currentUserCurrentHost | Add-Member -NotePropertyName '_name' -NotePropertyValue 'CurrentUserCurrentHost' -PassThru | ConvertTo-Json -Compress -EnumsAsStrings
    }

    function GetOperation {
        param (
            [Parameter(Mandatory = $true)]
            $InputResource,
            [Parameter()]
            [switch] $AsJson
        )

        $profilePath = GetProfilePath -profileType $InputResource.profileType.ToString()

        $actualState = New-PwshResource -ProfileType $InputResource.profileType

        $actualState.profilePath = $profilePath

        $exists = Test-Path $profilePath

        if ($InputResource._exist -and $exists) {
            $content = Get-Content -Path $profilePath
            $actualState.Content = $content
        } elseif ($InputResource._exist -and -not $exists) {
            $actualState.Content = $null
            $actualState._exist = $false
        } elseif (-not $InputResource._exist -and $exists) {
            $actualState.Content = Get-Content -Path $profilePath
            $actualState._exist = $true
        } else {
            $actualState.Content = $null
            $actualState._exist = $false
        }

        if ($AsJson) {
            return $actualState.ToJson()
        } else {
            return $actualState
        }
    }

    function SetOperation {
        param (
            $InputResource
        )

        $actualState = GetOperation -InputResource $InputResource

        if ($InputResource._exist) {
            if (-not $actualState._exist) {
                $null = New-Item -Path $actualState.profilePath -ItemType File -Force
            }

            if ($null -ne $InputResource.content) {
                Set-Content -Path $actualState.profilePath -Value $InputResource.content
            }
        } elseif ($actualState._exist) {
            Remove-Item -Path $actualState.profilePath -Force
        }
    }
}
End {
    $inputJson = $input | ConvertFrom-Json

    if ($inputJson) {
        $InputResource = New-PwshResource -ProfileType $inputJson.profileType -Content $inputJson.content -Exist $inputJson._exist
    }

    switch ($Operation) {
        'get' {
            GetOperation -InputResource $InputResource -AsJson
        }
        'set' {
            SetOperation -InputResource $InputResource
        }
        'export' {
            if ($inputJson) {
                Write-Error "Input not supported for export operation"
                exit 2
            }

            ExportOperation
        }
    }

    exit 0
}

# SIG # Begin signature block
# MIInKAYJKoZIhvcNAQcCoIInGTCCJxUCAQExDzANBglghkgBZQMEAgEFADB5Bgor
# BgEEAYI3AgEEoGswaTA0BgorBgEEAYI3AgEeMCYCAwEAAAQQH8w7YFlLCE63JNLG
# KX7zUQIBAAIBAAIBAAIBAAIBADAxMA0GCWCGSAFlAwQCAQUABCByJLrmewddsBTG
# 7RhmZpu3+7n6ngfUChFqTBK/Ws+SaKCCDLowggX1MIID3aADAgECAhMzAAACHU0Z
# yE7XD1dIAAAAAAIdMA0GCSqGSIb3DQEBCwUAMFcxCzAJBgNVBAYTAlVTMR4wHAYD
# VQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xKDAmBgNVBAMTH01pY3Jvc29mdCBD
# b2RlIFNpZ25pbmcgUENBIDIwMjQwHhcNMjYwNDE2MTg1OTQzWhcNMjcwNDE1MTg1
# OTQzWjB0MQswCQYDVQQGEwJVUzETMBEGA1UECBMKV2FzaGluZ3RvbjEQMA4GA1UE
# BxMHUmVkbW9uZDEeMBwGA1UEChMVTWljcm9zb2Z0IENvcnBvcmF0aW9uMR4wHAYD
# VQQDExVNaWNyb3NvZnQgQ29ycG9yYXRpb24wggEiMA0GCSqGSIb3DQEBAQUAA4IB
# DwAwggEKAoIBAQDQvewXxx9gZZFC6Ys1WBay8BJ8kGA4JQnH5CMafqOASlTpK9H8
# o5ZXTXt0caVQTNMUPt445wXYD+dFtaKWTwDn1I52oUSrC9vJin1Gsqt+zyKJL5Dg
# 3eQXbQNR61DmMy20GLTIO3SFed9Rfi/ophgCLGFLDR3r0KvHjwMb/jYWS0celV/4
# Lz27LfAekm8v9E5IXaeiXbAUYZKK090n4CVl3JBtbN+9DtI9SNu/yjvozW52/u7R
# X/Ttpa/KDlpuokZ+Zcbvmtd9ur9gFLvZzh41o9MsE/clQtdaFWGvuo6Jua/ntpgk
# ey3E5/vBFe+MJPG6phdnuo6r57ZudCudiI1bAgMBAAGjggGbMIIBlzAOBgNVHQ8B
# Af8EBAMCB4AwHwYDVR0lBBgwFgYKKwYBBAGCN0wIAQYIKwYBBQUHAwMwHQYDVR0O
# BBYEFH6QuMwqcPG0hQlQ6c5jCtTTLrVeMEUGA1UdEQQ+MDykOjA4MR4wHAYDVQQL
# ExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xFjAUBgNVBAUTDTIzMDAxMis1MDc1NTkw
# HwYDVR0jBBgwFoAUf1k/VCHarU/vBeXmo9ctBpQSCDEwYAYDVR0fBFkwVzBVoFOg
# UYZPaHR0cDovL3d3dy5taWNyb3NvZnQuY29tL3BraW9wcy9jcmwvTWljcm9zb2Z0
# JTIwQ29kZSUyMFNpZ25pbmclMjBQQ0ElMjAyMDI0LmNybDBtBggrBgEFBQcBAQRh
# MF8wXQYIKwYBBQUHMAKGUWh0dHA6Ly93d3cubWljcm9zb2Z0LmNvbS9wa2lvcHMv
# Y2VydHMvTWljcm9zb2Z0JTIwQ29kZSUyMFNpZ25pbmclMjBQQ0ElMjAyMDI0LmNy
# dDAMBgNVHRMBAf8EAjAAMA0GCSqGSIb3DQEBCwUAA4ICAQBKTbYOjzwTG/DXGaz9
# s6+fQeaTtDcFmMY+5UyVFCyj7Pv+5i37qfX8lSL/tBIfYQfWsMuBQlfZurJD6r4H
# VJ2CeH+1fgiq8dcHdVKoZ3Sa2qXoX3cq9iS8cVb06B7+5/XJ7I0OxHH9fDsvJ3T3
# w5V/ZtAIFmLrl+P0CtG+92uzRsn0nTbdFjOkLMLWPLAU3THohKRlSEMgFJpPkm5n
# 5UAZ35xX6FWCrDLsSKb555bTifwa8mJBwdlof0bmfYidH+dxZ1FdDxvLnNl9zeKs
# A4kejaaIqqIPguhwAti5Ql7BlTNoJNwxCvBmqW2MQLnCkYN/VVUsR3V2x/rcTNzo
# Bf/Z/SpROvdaA2ZOOd1uioXJt3tdLQ7vHpqpib0KfWr/FWXW10q38VxfCnRQBqzb
# SuztR7nEMuzX7Ck+B/XaPDXd1qh72+QYyB0Z2VzWmO9zsnb9Uq/dwu8LGeQqnyu6
# 7SDGACvnXii2fb9+US492VTnXSnFKyqwgzUyFMtZK1/sHYTv6bG4TtQUygQxTN+Z
# V+aJIlKO2MqZ7bKrAnOzS9m6NgoTdWOq11bTOZwKlIEV/EhV9SWkDmdpR/hPPT2v
# 6TEj4F8PT/zHjRezIU5c/DGlt/VhY/pK0XkJtEyMmmS1BMtjU/rqBZVMIm3dnxQs
# /TBByr+Cf8Z1r7aifQVQ+WSqzjCCBr0wggSloAMCAQICEzMAAAA5O7Y3Gb8GHWcA
# AAAAADkwDQYJKoZIhvcNAQEMBQAwgYgxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpX
# YXNoaW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQg
# Q29ycG9yYXRpb24xMjAwBgNVBAMTKU1pY3Jvc29mdCBSb290IENlcnRpZmljYXRl
# IEF1dGhvcml0eSAyMDExMB4XDTI0MDgwODIwNTQxOFoXDTM2MDMyMjIyMTMwNFow
# VzELMAkGA1UEBhMCVVMxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEo
# MCYGA1UEAxMfTWljcm9zb2Z0IENvZGUgU2lnbmluZyBQQ0EgMjAyNDCCAiIwDQYJ
# KoZIhvcNAQEBBQADggIPADCCAgoCggIBANgBnB7jOMeqlRYHNa265v4IY9fH8TKh
# emHfPINe1gpLaV3dhg324WwH06LcHbpnsBukCDNitryo0dtS/EW6I/yEL/bLSY8h
# KpbfQuWusBPr9qazYcDxCW/qnjb5JsI1s8bNOg3bVATvQVL4tcf03aTycsz8QeCd
# M0l/yHRObJ9QqazM1r6VPEOJ7LL+uEEb73w6QCuhs89a1uv1zerOYMnsneRRwCbp
# yW11IcggU0cRKDDq1pjVJzIbIF6+oiXXbReOsgeI8zu1FyQfK0fVkaya8SmVHQ/t
# Of23mZ4W9k0Ri22QW9p3UgSC5OUDktKxxcCmGL6tXLfOGSWHIIV4YrTJTT6PNty5
# REojHJuZHArkF9VnHTERWoTjAzfI3kP+5b4alUdhgAZ7ttOu1bVnXfHaqPYl2rPs
# 20ji03LOVWsh/radgE17es5hL+t6lV0eVHrVhsssROWJuz2MXMCt7iw7lFPG9LXK
# Gjsmonn2gotGdHIuEg5JnJMJVmixd5LRlkmgYRZKzhxSCwyoGIq0PhaA7Y+VPct5
# pCHkijcIIDm0nlkK+0KyepolcqGm0T/GYQRMhHJlGOOmVQop36wUVUYklUy++vDW
# eEgEo4s7hxN6mIbf2MSIQ/iIfMZgJxC69oukMUXCrOC3SkE/xIkgpfl22MM1itkZ
# 35nNXkMolU1lAgMBAAGjggFOMIIBSjAOBgNVHQ8BAf8EBAMCAYYwEAYJKwYBBAGC
# NxUBBAMCAQAwHQYDVR0OBBYEFH9ZP1Qh2q1P7wXl5qPXLQaUEggxMBkGCSsGAQQB
# gjcUAgQMHgoAUwB1AGIAQwBBMA8GA1UdEwEB/wQFMAMBAf8wHwYDVR0jBBgwFoAU
# ci06AjGQQ7kUBU7h6qfHMdEjiTQwWgYDVR0fBFMwUTBPoE2gS4ZJaHR0cDovL2Ny
# bC5taWNyb3NvZnQuY29tL3BraS9jcmwvcHJvZHVjdHMvTWljUm9vQ2VyQXV0MjAx
# MV8yMDExXzAzXzIyLmNybDBeBggrBgEFBQcBAQRSMFAwTgYIKwYBBQUHMAKGQmh0
# dHA6Ly93d3cubWljcm9zb2Z0LmNvbS9wa2kvY2VydHMvTWljUm9vQ2VyQXV0MjAx
# MV8yMDExXzAzXzIyLmNydDANBgkqhkiG9w0BAQwFAAOCAgEAFJQfOChP7onn6fLI
# MKrSlN1WYKwDFgAddymOUO3FrM8d7B/W/iQ6DxXsDn7D5W4wMwYeLystcEqfkjz4
# NURRgazyMu5yRzQh4LqjA4tStTcJh1opExo7nn5PuPBYnbu0+THSuVHTe0VTTPVh
# ily/piFrDo3axQ9P4C+Ol5yet+2gTfekICS5xS+cYfSIvgn0JksVBVMYVI5QFu/q
# hnLhsEFEUzG8fvv0hjgkO+lkpV9ty6GkN4vdnd7ya6Q6aR9y34aiM1qmxaxBi6OU
# nyNl6fkuun/diTFnYDLTppOkr/mg5WSfCiDVMNCxtj4wPKC5OmHm1DQIt/MNokbb
# H3UGsFP1QbzsLocuSqLCvH09Io3fDPTmscR9Y75G4qX7RTX8AdBPo0I6OEojf39z
# uFZt0qOHm65YWQE69cZM2ueE1MB05dNNgHK9gTE7zKvK/fg8B2qjW88MT/WF5V5u
# vZGtqa9FSL2RazArA+rDPuf6JGYz4HpgMZHB4S6szWSKYBv0VisCzfxgeU+dquXW
# 9bd0auYlOB58DPcOYKdc3Se94g+xL4pcEhbB54JOgAkwYTu/9dLeH2pDqeJZAABV
# DWRQCaXfO5LgyKwKCLYXpigrZYCjUSBcr+Ve8PFWMhVTQl0v4q8J/AUmQN5W4n10
# 1cY2L4A7GTQG1h32HHAvfQESWP0xghnEMIIZwAIBATBuMFcxCzAJBgNVBAYTAlVT
# MR4wHAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xKDAmBgNVBAMTH01pY3Jv
# c29mdCBDb2RlIFNpZ25pbmcgUENBIDIwMjQCEzMAAAIdTRnITtcPV0gAAAAAAh0w
# DQYJYIZIAWUDBAIBBQCggZAwGQYJKoZIhvcNAQkDMQwGCisGAQQBgjcCAQQwLwYJ
# KoZIhvcNAQkEMSIEIJpzkwdoxpb2+tSsoFjmFoiV37jRJCGrYb29mF3i7GhyMEIG
# CisGAQQBgjcCAQwxNDAyoBSAEgBNAGkAYwByAG8AcwBvAGYAdKEagBhodHRwOi8v
# d3d3Lm1pY3Jvc29mdC5jb20wDQYJKoZIhvcNAQEBBQAEggEAWGFO/Klor4YrAIkT
# Dddw2C5OFXwm+NbWs8i7IUzpbtVp0DP4pRexZEIA3J7MxEEWH6F9oKZoYNCtYZOh
# xp4BysSEuJXRCtY4VKwk1t2EBnGb5Scwo/QnaDwCQ1TKFE1fwLEME1i19nbiZeNF
# YmebeaabEkw2s2ivhNL20XJpLiGcAcSoWe+xynMwFjbiA/9R2TNKxQfzqwQzly5o
# ANy9mCpYmUE20MPYtCdtJu+kQfP/weyAasm8tbxXaXJs/8gd5izI6Msxju0t1ViB
# aDlD29eE9GM2GpBfwZLrmXN8/InEZQ7TFGNevzp3y0aJyu3tJKBRF54+7tXiVxn3
# dAjOiaGCF5QwgheQBgorBgEEAYI3AwMBMYIXgDCCF3wGCSqGSIb3DQEHAqCCF20w
# ghdpAgEDMQ8wDQYJYIZIAWUDBAIBBQAwggFSBgsqhkiG9w0BCRABBKCCAUEEggE9
# MIIBOQIBAQYKKwYBBAGEWQoDATAxMA0GCWCGSAFlAwQCAQUABCBn0wN+pD0W8H7J
# 4bpIH/mBzAQwqrR2iTcebX4O9reUnwIGaoUMaNTTGBMyMDI2MDkwMjE5MjgyNy41
# MjFaMASAAgH0oIHRpIHOMIHLMQswCQYDVQQGEwJVUzETMBEGA1UECBMKV2FzaGlu
# Z3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEeMBwGA1UEChMVTWljcm9zb2Z0IENvcnBv
# cmF0aW9uMSUwIwYDVQQLExxNaWNyb3NvZnQgQW1lcmljYSBPcGVyYXRpb25zMScw
# JQYDVQQLEx5uU2hpZWxkIFRTUyBFU046N0YwMC0wNUUwLUQ5NDcxJTAjBgNVBAMT
# HE1pY3Jvc29mdCBUaW1lLVN0YW1wIFNlcnZpY2WgghHqMIIHIDCCBQigAwIBAgIT
# MwAAAh6jrKRuOW98SQABAAACHjANBgkqhkiG9w0BAQsFADB8MQswCQYDVQQGEwJV
# UzETMBEGA1UECBMKV2FzaGluZ3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEeMBwGA1UE
# ChMVTWljcm9zb2Z0IENvcnBvcmF0aW9uMSYwJAYDVQQDEx1NaWNyb3NvZnQgVGlt
# ZS1TdGFtcCBQQ0EgMjAxMDAeFw0yNjAyMTkxOTM5NDlaFw0yNzA1MTcxOTM5NDla
# MIHLMQswCQYDVQQGEwJVUzETMBEGA1UECBMKV2FzaGluZ3RvbjEQMA4GA1UEBxMH
# UmVkbW9uZDEeMBwGA1UEChMVTWljcm9zb2Z0IENvcnBvcmF0aW9uMSUwIwYDVQQL
# ExxNaWNyb3NvZnQgQW1lcmljYSBPcGVyYXRpb25zMScwJQYDVQQLEx5uU2hpZWxk
# IFRTUyBFU046N0YwMC0wNUUwLUQ5NDcxJTAjBgNVBAMTHE1pY3Jvc29mdCBUaW1l
# LVN0YW1wIFNlcnZpY2UwggIiMA0GCSqGSIb3DQEBAQUAA4ICDwAwggIKAoICAQCl
# 0TjtbDwsR7Fe8ac6ol5s1zhtTqd2AWpchQhLp9G5mmSM23N5fyQGCQ1D06rOA3Pg
# XKF+76vXvOCs2VsLv1owj4mHEyEqiq8GJ5yC+/QNYRpZPA8e7OgekzDO6S/4vy/j
# TMYbp3rhuFiKKCzTWOQtdFcF+D0k369I7pm/E07SyNMGkuNd5lj5SJ91UqFuZfjM
# B6cQ2wh77mtiRUVdj53yjdNqj+GQl+Yaz29Bjrzn7U1ln+JpLlnb0xdGmZoIPKZb
# wBVcWtyL4uyhML7SSTmiOfWXU+g+yNl0CdoLGL8LtWHEi8FsuTPeSdSqmeMrvLaE
# mibTVTS4vQQY8NPnb6uI5y6iNV9vBFcm8LU/lDTjGTqPa7UBT4gdf5Jm3wYrfCFZ
# 4P/j5MoqT0JONca50jt4TGI90SihXaDEYqk23S0IJZ3UkUpukDRTjK713BIykffx
# yBqMeQqfO0zvWfUx7BrmUpugQcw99+DxLl2gf+uQEpRmnlbrVJ9dvW9ds4fqEPN2
# jG0QwF1PBSglNcV1SpqZKitQgBGSwu/82AKztoCHwYRHRNwzwTVe/1KNTvmqAd4U
# ges4ywOH02haagT8wYY8OdWdjKn3k052w+kmc0UC0F+iVXTGZIMxvo9iBZQoXehz
# RtWJ/VOtKvCyS3csKzN7rStWJwjSWz6dtOf0l+ytLQIDAQABo4IBSTCCAUUwHQYD
# VR0OBBYEFOYKFprqBB0JZmJcFC4cPPmeF4JkMB8GA1UdIwQYMBaAFJ+nFV0AXmJd
# g/Tl0mWnG1M1GelyMF8GA1UdHwRYMFYwVKBSoFCGTmh0dHA6Ly93d3cubWljcm9z
# b2Z0LmNvbS9wa2lvcHMvY3JsL01pY3Jvc29mdCUyMFRpbWUtU3RhbXAlMjBQQ0El
# MjAyMDEwKDEpLmNybDBsBggrBgEFBQcBAQRgMF4wXAYIKwYBBQUHMAKGUGh0dHA6
# Ly93d3cubWljcm9zb2Z0LmNvbS9wa2lvcHMvY2VydHMvTWljcm9zb2Z0JTIwVGlt
# ZS1TdGFtcCUyMFBDQSUyMDIwMTAoMSkuY3J0MAwGA1UdEwEB/wQCMAAwFgYDVR0l
# AQH/BAwwCgYIKwYBBQUHAwgwDgYDVR0PAQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUA
# A4ICAQCkoZB5NnJVFb5wKejRonk518a2TBNYpKcBMtfL6BS0ARaABOMGYLlPNuhI
# 1HwmelP9hX3oq3TaEm/cDkkzNQAzDedPgoRI2R7+8poNSWvHXEAs7SZODm9x7Kql
# BkNZM9ex4XY1yNmVOAmWDjRr7jKjaiQbntf7EC4GNikxGGaVWOjfYt3Q9X0r/Ks8
# KBlbzDR9zjA/TCctR4co1WpU1ZRLFrB9bl8dRxsbnyT2qQ41E7dT12R30eIGUziE
# s5GN+26V/ovXOi20dJiM13hYWvy1NNJAhkKOlLB1ONund6ffhPdUcHWsu8V+lR0a
# akMV64HqDbLumZrCNwUofVx3xMk8F4tCYJtQxLTywc30sZAD1S2sC1959x6KixA+
# p41FLUl8g64oHy3bfYnH5xd4JOBgQoaqndGjcctxr+8EknjhKyrgAzrTcKLJbUez
# goye8brCLJ+y6PAoEjpXRkSYAU8wfQ3YWRck6ALwoV7Uin8+rpGQSbXhF6c1dTFa
# kXmChClud4IADY/t6JRkJ+06FzL+jDd8KLV8Qj77JfiuTiPIG5G/xlnGoZFcX+yy
# BtDvzZE48d+Y+HYUd/cvhH1FKl7AH+5AyotqJSFmvM/BuYRx2B20asVXilV2k2Jb
# NO3LGCz3Q+dpElzwsfJrka1N/getma7fWpowsNvoIaEQvjad8TCCB3EwggVZoAMC
# AQICEzMAAAAVxedrngKbSZkAAAAAABUwDQYJKoZIhvcNAQELBQAwgYgxCzAJBgNV
# BAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4w
# HAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xMjAwBgNVBAMTKU1pY3Jvc29m
# dCBSb290IENlcnRpZmljYXRlIEF1dGhvcml0eSAyMDEwMB4XDTIxMDkzMDE4MjIy
# NVoXDTMwMDkzMDE4MzIyNVowfDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hp
# bmd0b24xEDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jw
# b3JhdGlvbjEmMCQGA1UEAxMdTWljcm9zb2Z0IFRpbWUtU3RhbXAgUENBIDIwMTAw
# ggIiMA0GCSqGSIb3DQEBAQUAA4ICDwAwggIKAoICAQDk4aZM57RyIQt5osvXJHm9
# DtWC0/3unAcH0qlsTnXIyjVX9gF/bErg4r25PhdgM/9cT8dm95VTcVrifkpa/rg2
# Z4VGIwy1jRPPdzLAEBjoYH1qUoNEt6aORmsHFPPFdvWGUNzBRMhxXFExN6AKOG6N
# 7dcP2CZTfDlhAnrEqv1yaa8dq6z2Nr41JmTamDu6GnszrYBbfowQHJ1S/rboYiXc
# ag/PXfT+jlPP1uyFVk3v3byNpOORj7I5LFGc6XBpDco2LXCOMcg1KL3jtIckw+DJ
# j361VI/c+gVVmG1oO5pGve2krnopN6zL64NF50ZuyjLVwIYwXE8s4mKyzbnijYjk
# lqwBSru+cakXW2dg3viSkR4dPf0gz3N9QZpGdc3EXzTdEonW/aUgfX782Z5F37Zy
# L9t9X4C626p+Nuw2TPYrbqgSUei/BQOj0XOmTTd0lBw0gg/wEPK3Rxjtp+iZfD9M
# 269ewvPV2HM9Q07BMzlMjgK8QmguEOqEUUbi0b1qGFphAXPKZ6Je1yh2AuIzGHLX
# pyDwwvoSCtdjbwzJNmSLW6CmgyFdXzB0kZSU2LlQ+QuJYfM2BjUYhEfb3BvR/bLU
# HMVr9lxSUV0S2yW6r1AFemzFER1y7435UsSFF5PAPBXbGjfHCBUYP3irRbb1Hode
# 2o+eFnJpxq57t7c+auIurQIDAQABo4IB3TCCAdkwEgYJKwYBBAGCNxUBBAUCAwEA
# ATAjBgkrBgEEAYI3FQIEFgQUKqdS/mTEmr6CkTxGNSnPEP8vBO4wHQYDVR0OBBYE
# FJ+nFV0AXmJdg/Tl0mWnG1M1GelyMFwGA1UdIARVMFMwUQYMKwYBBAGCN0yDfQEB
# MEEwPwYIKwYBBQUHAgEWM2h0dHA6Ly93d3cubWljcm9zb2Z0LmNvbS9wa2lvcHMv
# RG9jcy9SZXBvc2l0b3J5Lmh0bTATBgNVHSUEDDAKBggrBgEFBQcDCDAZBgkrBgEE
# AYI3FAIEDB4KAFMAdQBiAEMAQTALBgNVHQ8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB
# /zAfBgNVHSMEGDAWgBTV9lbLj+iiXGJo0T2UkFvXzpoYxDBWBgNVHR8ETzBNMEug
# SaBHhkVodHRwOi8vY3JsLm1pY3Jvc29mdC5jb20vcGtpL2NybC9wcm9kdWN0cy9N
# aWNSb29DZXJBdXRfMjAxMC0wNi0yMy5jcmwwWgYIKwYBBQUHAQEETjBMMEoGCCsG
# AQUFBzAChj5odHRwOi8vd3d3Lm1pY3Jvc29mdC5jb20vcGtpL2NlcnRzL01pY1Jv
# b0NlckF1dF8yMDEwLTA2LTIzLmNydDANBgkqhkiG9w0BAQsFAAOCAgEAnVV9/Cqt
# 4SwfZwExJFvhnnJL/Klv6lwUtj5OR2R4sQaTlz0xM7U518JxNj/aZGx80HU5bbsP
# MeTCj/ts0aGUGCLu6WZnOlNN3Zi6th542DYunKmCVgADsAW+iehp4LoJ7nvfam++
# Kctu2D9IdQHZGN5tggz1bSNU5HhTdSRXud2f8449xvNo32X2pFaq95W2KFUn0CS9
# QKC/GbYSEhFdPSfgQJY4rPf5KYnDvBewVIVCs/wMnosZiefwC2qBwoEZQhlSdYo2
# wh3DYXMuLGt7bj8sCXgU6ZGyqVvfSaN0DLzskYDSPeZKPmY7T7uG+jIa2Zb0j/aR
# AfbOxnT99kxybxCrdTDFNLB62FD+CljdQDzHVG2dY3RILLFORy3BFARxv2T5JL5z
# bcqOCb2zAVdJVGTZc9d/HltEAY5aGZFrDZ+kKNxnGSgkujhLmm77IVRrakURR6nx
# t67I6IleT53S0Ex2tVdUCbFpAUR+fKFhbHP+CrvsQWY9af3LwUFJfn6Tvsv4O+S3
# Fb+0zj6lMVGEvL8CwYKiexcdFYmNcP7ntdAoGokLjzbaukz5m/8K6TT4JDVnK+AN
# uOaMmdbhIurwJ0I9JZTmdHRbatGePu1+oDEzfbzL6Xu/OHBE0ZDxyKs6ijoIYn/Z
# cGNTTY3ugm2lBRDBcQZqELQdVTNYs6FwZvKhggNNMIICNQIBATCB+aGB0aSBzjCB
# yzELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0b24xEDAOBgNVBAcTB1Jl
# ZG1vbmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjElMCMGA1UECxMc
# TWljcm9zb2Z0IEFtZXJpY2EgT3BlcmF0aW9uczEnMCUGA1UECxMeblNoaWVsZCBU
# U1MgRVNOOjdGMDAtMDVFMC1EOTQ3MSUwIwYDVQQDExxNaWNyb3NvZnQgVGltZS1T
# dGFtcCBTZXJ2aWNloiMKAQEwBwYFKw4DAhoDFQCD/QNkKDIW4VIF7j3oi2qbrR0a
# /6CBgzCBgKR+MHwxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAw
# DgYDVQQHEwdSZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24x
# JjAkBgNVBAMTHU1pY3Jvc29mdCBUaW1lLVN0YW1wIFBDQSAyMDEwMA0GCSqGSIb3
# DQEBCwUAAgUA7kKmPDAiGA8yMDI2MDkwMjEzNDIyMFoYDzIwMjYwOTAzMTM0MjIw
# WjB0MDoGCisGAQQBhFkKBAExLDAqMAoCBQDuQqY8AgEAMAcCAQACAiSaMAcCAQAC
# AhL9MAoCBQDuQ/e8AgEAMDYGCisGAQQBhFkKBAIxKDAmMAwGCisGAQQBhFkKAwKg
# CjAIAgEAAgMHoSChCjAIAgEAAgMBhqAwDQYJKoZIhvcNAQELBQADggEBAESnmR5W
# RhZmRaIyI2M8uzQ0OIgywP2pQGVg6lx18l5cR+dRIgDcBPT5jwrya/Abo8PQzWJf
# fSqeieN8uN1erivDDaItleJUP3PnMSk1PHkepLsIfA+iUnHWCSiOZMfKXI4tyxDe
# hYo/89bE87GQ6mVDXdiJsG++ix1qQ0FVpZgFKto5APBNLGtyZx2owmGI3BeLngu/
# H1/tLenyoPFAbpUZ5I49ZqL3myn3FqQ7ox4Tzb43CQJvhCEwgZfOFqLNUGK7nZLy
# yJNL6bVPB4th1DP0U/X+ta5vaw+qPrRLkuBY8jAooItCLjKh9DMvPgpJ1C47ojYk
# nUQJ/Gioqjc3wT0xggQNMIIECQIBATCBkzB8MQswCQYDVQQGEwJVUzETMBEGA1UE
# CBMKV2FzaGluZ3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEeMBwGA1UEChMVTWljcm9z
# b2Z0IENvcnBvcmF0aW9uMSYwJAYDVQQDEx1NaWNyb3NvZnQgVGltZS1TdGFtcCBQ
# Q0EgMjAxMAITMwAAAh6jrKRuOW98SQABAAACHjANBglghkgBZQMEAgEFAKCCAUow
# GgYJKoZIhvcNAQkDMQ0GCyqGSIb3DQEJEAEEMC8GCSqGSIb3DQEJBDEiBCAjSppx
# g0/R57fw5wZSi++4vy4jRrjT9LPP7W08TGHSNTCB+gYLKoZIhvcNAQkQAi8xgeow
# gecwgeQwgb0EIC+BXWrz9geMgM8Bvn8bqxHjhHXJ29EBizITIw0B9vOCMIGYMIGA
# pH4wfDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0b24xEDAOBgNVBAcT
# B1JlZG1vbmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEmMCQGA1UE
# AxMdTWljcm9zb2Z0IFRpbWUtU3RhbXAgUENBIDIwMTACEzMAAAIeo6ykbjlvfEkA
# AQAAAh4wIgQg20x8alpDQTAu240oUYEKrhrRQjNWlzQWT+1stbYI7nswDQYJKoZI
# hvcNAQELBQAEggIAHRJIPq3/jc2tG7mOYDABaPKeLSJiObetnOt55Qhe7Geis+9b
# Fca1xtXRqbNawYF+Dn9CicTDjcqj12uGiKk1duGP8X3qRpcttRG+N6CTauFkcj5W
# hRf2DLNMqhGWmdzPRsSVudaJsDJilKDPa9qK1P0Wm42FaY4GlJRgvc/pIbFDqiRs
# KGxSN/+fXUTdTg2QHho84aWQ7XGoEtEn7++DeYUa2MXeCS893eay7DnrvLg97uob
# Zk5OBqJUkJ1knxcJ9VY+JoWtXJW3XciSHfpw2XSVJKJGFfipDcyLrgHxhY0HW6GQ
# JyLXJrwN9DHjz9xQOVUwPPRDIFzct9OclYLVMAk8Q3kU/dQphdSYkOcRqD+54jl2
# VUCe8CRKVj5Ye+9SV6XXEvKl4wZqg3U56ENKlgqXOx1o18FyuEv+uqt/Q9/MyWRl
# L2/hkARQALlnae7E7NVklMUmmhyrVf4VU0i5BCl9fRZVmTP0gBCxfKTR/TqHW0lI
# YKOg9oH3HvHSF21Jo6tXg1Lus0n3MZvq6d4Xub5p78Dghn9bhq6RK6VWoSvqtO5i
# zTvgljZcCplkKrxFQYZ4NlziVQwrhUt22uKQUyfA9Tg2UApuAYpDk0U8a0V0W0t8
# MMeTcX4v4UFNvCCpWiPZSBwC8fBgDQiRh3raJTSi10aiVouomKbpWTTBI3s=
# SIG # End signature block
