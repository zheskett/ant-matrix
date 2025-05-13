import { Assets, Container, Graphics, Ticker } from "pixi.js";
import { IScene, Manager } from "../util/manager";
import { manifest } from "../util/assets";

export class LoadScene extends Container implements IScene {
  private loaderBar: Container;
  private loaderBarBorder: Graphics;
  private loaderBarFill: Graphics;

  private scene: new () => IScene;

  constructor(scene: new () => IScene) {
    super();
    this.scene = scene;

    const loaderBarWidth = Manager.width * 0.7;
    const loaderBarHeight = 40;

    this.loaderBar = new Container();

    this.loaderBarFill = new Graphics();
    this.loaderBarFill.rect(0, 0, loaderBarWidth, loaderBarHeight);
    this.loaderBarFill.fill(0xee0000);
    this.loaderBarFill.scale.x = 0;

    this.loaderBarBorder = new Graphics();
    this.loaderBarBorder.rect(0, 0, loaderBarWidth, loaderBarHeight);
    this.loaderBarBorder.stroke({ width: 10, color: 0x0 });

    this.loaderBar = new Container();
    this.loaderBar.addChild(this.loaderBarFill);
    this.loaderBar.addChild(this.loaderBarBorder);
    this.loaderBar.position.x = (Manager.width - this.loaderBar.width) / 2;
    this.loaderBar.position.y = (Manager.height - this.loaderBar.height) / 2;
    this.addChild(this.loaderBar);

    this.initialize().then(() => {
      Manager.setScene(new this.scene());
    });
  }

  private async initialize(): Promise<void> {
    await Assets.init({ manifest });

    const bundleIds = manifest.bundles.map((bundle) => bundle.name);
    await Assets.loadBundle(bundleIds, this.updateLoaderBar.bind(this));
  }

  private updateLoaderBar(progress: number): void {
    this.loaderBarFill.scale.x = progress;
  }

  public update(_time: Ticker): void {
    // Not used in this scene
  }
  public fixedUpdate(_time: { deltaTime: number; FPS: number }): void {
    // Not used in this scene
  }
}
