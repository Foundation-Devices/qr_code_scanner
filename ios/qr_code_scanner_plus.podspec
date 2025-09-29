#
# To learn more about a Podspec see http://guides.cocoapods.org/syntax/podspec.html
#

Pod::Spec.new do |s|
  s.name             = 'qr_code_scanner_plus'
  s.version          = '0.2.6'
  s.summary          = 'QR Code Scanner for flutter.'
  s.description      = <<-DESC
A new Flutter project.
                       DESC
  s.homepage         = 'https://github.com/vespr-wallet/qr_code_scanner_plus'
  s.license          = { :file => '../LICENSE' }
  s.author           = { 'Your Company' => 'all[*]vespr.xyz' }
  s.source           = { :path => '.' }
  s.source_files = 'Classes/**/*'
  s.public_header_files = 'Classes/**/*.h'
  s.dependency 'Flutter'
  s.dependency 'MTBBarcodeScanner'
  s.ios.deployment_target = '8.0'
  s.swift_version = '4.0'
end

