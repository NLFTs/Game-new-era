extends CanvasLayer

signal start_game
# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	pass # Replace with function body.

func show_message(text):
	$Message.text = text
	$Message.show()
	$MessageTimer.start()
func show_game_over():
	show_message("Game Over")
	await $MessageTimer.timeout

	$Message.text = "Dodge the creep !"
	$Message.show()

	await get_tree().create_timer(1.0).timeout
	$startButton.show()
func update_score(score):
	$ScoreLabel.text = str(score)
func _on_start_button_pressed():
	$startButton.hide()
	start_game.emit()
func _on_message_timer_timeout():
	$Message.hide()
