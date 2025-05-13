import { Ticker } from "pixi.js";

const defaultFps = 60;

export class Physics {
  private static ticker: Ticker;

  private constructor() {}

  public static initialize(callback: (time: Ticker) => void, fps?: number): void {
    if (Physics.ticker) {
      throw new Error("Physics is already initialized");
    }
    Physics.ticker = new Ticker();
    Physics.ticker.autoStart = false;

    Physics.ticker.minFPS = fps || defaultFps;
    Physics.ticker.maxFPS = fps || defaultFps;
    Physics.ticker.start();

    Physics.ticker.add(callback);
  }

  public static destroy(): void {
    if (!Physics.ticker) {
      return;
    }

    Physics.ticker.stop();
    Physics.ticker.destroy();
    Physics.ticker = undefined;
  }
}
