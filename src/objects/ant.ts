import { Container, Sprite } from "pixi.js";

export class Ant extends Container {
  private sprite: Sprite;

  constructor() {
    super();

    this.sprite = Sprite.from("ant");
    this.sprite.anchor.set(0.5);
    this.sprite.position.set(0, 0);
    this.sprite.scale.set(2);
    this.addChild(this.sprite);
  }
}
