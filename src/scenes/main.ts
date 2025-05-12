import { Assets, Container, Sprite, Ticker } from "pixi.js";
import { IScene, Manager } from "../util/manager";

export class MainScene extends Container implements IScene {
  private ant: Sprite;

  constructor() {
    super();

    this.initialize();
  }

  private async initialize(): Promise<void> {
    const antTexture = await Assets.load("assets/ant.png");
    this.ant = new Sprite(antTexture);

    this.ant.anchor.set(0.5);
    this.ant.position.set(Manager.width / 2, Manager.height / 2);
    this.addChild(this.ant);
  }

  public update(time: Ticker): void {
    if (!this.ant) return;

    this.ant.rotation += 0.1 * time.deltaTime;
  }
}
