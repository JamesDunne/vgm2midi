GameNotes::GameNotes(TabFrame* parent) : TabFrameItem(parent) {
  setIcon(Icon::Emblem::Text);
  setText("Game Notes");

  layout.setPadding(5);
  notes.setWordWrap(false).setFont(Font().setFamily(Font::Mono));
}

auto GameNotes::loadNotes() -> void {
  auto contents = string::read({program->gamePaths(1), "higan/notes.txt"});
  notes.setText(contents);
}

auto GameNotes::saveNotes() -> void {
  auto contents = notes.text();
  if(contents) {
    directory::create({program->gamePaths(1), "higan/"});
    file::write({program->gamePaths(1), "higan/notes.txt"}, contents);
  } else {
    file::remove({program->gamePaths(1), "higan/notes.txt"});
  }
}
