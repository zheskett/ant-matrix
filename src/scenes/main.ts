import { Container, Ticker } from "pixi.js";
import { IScene, Manager } from "../util/manager";
import { Ant } from "../objects/ant";

export class MainScene extends Container implements IScene {
  private ant: Ant;
  private last500dt: number[] = [];

  constructor() {
    super();

    this.initialize();
  }

  private initialize(): void {
    this.ant = new Ant();
    this.ant.position.set(Manager.width / 2, Manager.height / 2);
    this.addChild(this.ant);
  }

  public update(time: Ticker): void {
    this.ant.rotation += 0.01 * time.deltaTime;
  }

  public fixedUpdate(time: Ticker): void {
    this.last500dt.push(time.FPS);
    if (this.last500dt.length > 500) {
      this.last500dt.shift();
    }
    console.log(`FPS: ${time.FPS}, Average FPS: ${this.last500dt.reduce((a, b) => a + b, 0) / this.last500dt.length}`);
  }
}
