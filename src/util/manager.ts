import { Application, Container, Ticker } from "pixi.js";

const DEFAULT_WIDTH = 1920;
const DEFAULT_HEIGHT = 1080;
const DEFAULT_BACKGROUND = 0xb5651d;

export interface IScene extends Container {
  update(time: Ticker): void;
}

export class Manager {
  private constructor() {}

  private static app: Application;
  private static currentScene: IScene;

  private static _width: number;
  private static _height: number;

  public static get width(): number {
    return Manager._width;
  }
  public static get height(): number {
    return Manager._height;
  }

  public static async initialize(width?: number, height?: number, background?: number): Promise<void> {
    Manager._width = width || DEFAULT_WIDTH;
    Manager._height = height || DEFAULT_HEIGHT;

    Manager.app = new Application();
    await Manager.app.init({
      resolution: window.devicePixelRatio || 1,
      autoDensity: true,
      backgroundColor: background || DEFAULT_BACKGROUND,
      width: Manager._width,
      height: Manager._height,
    });
    document.getElementById("pixi-container")!.appendChild(Manager.app.canvas);

    Manager.app.ticker.add((time) => {
      if (Manager.currentScene) {
        Manager.currentScene.update(time);
      }
    });

    window.addEventListener("resize", Manager.resize);
    Manager.resize();
  }

  public static setScene(scene: IScene): void {
    if (Manager.currentScene) {
      Manager.app.stage.removeChild(Manager.currentScene);
      Manager.currentScene.destroy();
    }
    Manager.currentScene = scene;
    Manager.app.stage.addChild(Manager.currentScene);
  }

  public static resize(): void {
    if (Manager.app) {
      // current screen size
      const screenWidth = Math.max(document.documentElement.clientWidth, window.innerWidth || 0);
      const screenHeight = Math.max(document.documentElement.clientHeight, window.innerHeight || 0);

      // uniform scale
      const scale = Math.min(screenWidth / Manager.width, screenHeight / Manager.height);

      // the "uniformly enlarged" size
      const enlargedWidth = Math.floor(scale * Manager.width);
      const enlargedHeight = Math.floor(scale * Manager.height);

      // margins for centering
      const horizontalMargin = (screenWidth - enlargedWidth) / 2;
      const verticalMargin = (screenHeight - enlargedHeight) / 2;

      // now we use css trickery to set the sizes and margins
      Manager.app.canvas.style.width = `${enlargedWidth}px`;
      Manager.app.canvas.style.height = `${enlargedHeight}px`;
      Manager.app.canvas.style.marginLeft = Manager.app.canvas.style.marginRight = `${horizontalMargin}px`;
      Manager.app.canvas.style.marginTop = Manager.app.canvas.style.marginBottom = `${verticalMargin}px`;
    }
  }
}
