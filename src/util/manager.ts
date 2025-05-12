import { Application } from "pixi.js";

export class Manager {
  private constructor() {}

  private static app: Application;

  private static _width: number;
  private static _height: number;

  public static get width(): number {
    return Manager._width;
  }
  public static get height(): number {
    return Manager._height;
  }

  public static initialize(width: number, height: number): void {
    Manager._width = width;
    Manager._height = height;

    Manager.app = new Application();
  }
}
