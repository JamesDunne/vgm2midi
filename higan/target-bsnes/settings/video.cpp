auto VideoSettings::create() -> void {
  setIcon(Icon::Device::Display);
  setText("Video");

  layout.setPadding(5);

  colorAdjustmentLabel.setFont(Font().setBold()).setText("Color Adjustment");
  colorLayout.setSize({3, 3});
  colorLayout.column(0).setAlignment(1.0);
  luminanceLabel.setText("Luminance:");
  luminanceValue.setAlignment(0.5);
  luminanceSlider.setLength(101).setPosition(settings.video.luminance).onChange([&] {
    string value = {luminanceSlider.position(), "%"};
    settings.video.luminance = value.natural();
    luminanceValue.setText(value);
    program.updateVideoPalette();
  }).doChange();
  saturationLabel.setText("Saturation:");
  saturationValue.setAlignment(0.5);
  saturationSlider.setLength(201).setPosition(settings.video.saturation).onChange([&] {
    string value = {saturationSlider.position(), "%"};
    settings.video.saturation = value.natural();
    saturationValue.setText(value);
    program.updateVideoPalette();
  }).doChange();
  gammaLabel.setText("Gamma:");
  gammaValue.setAlignment(0.5);
  gammaSlider.setLength(101).setPosition(settings.video.gamma - 100).onChange([&] {
    string value = {100 + gammaSlider.position(), "%"};
    settings.video.gamma = value.natural();
    gammaValue.setText(value);
    program.updateVideoPalette();
  }).doChange();
}
