import { LoadScene } from "./scenes/load";
import { MainScene } from "./scenes/main";
import { Manager } from "./util/manager";

(async () => {
  await Manager.initialize();
  Manager.setScene(new LoadScene(MainScene));

  // Wait for background color to be applied
  await new Promise((resolve) => setTimeout(resolve, 5));
  document.getElementById("pixi-container")!.removeAttribute("style");
})();
