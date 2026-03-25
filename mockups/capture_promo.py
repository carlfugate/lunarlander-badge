"""Capture promo-hero.html as a short looping video."""
import asyncio
from pathlib import Path
from playwright.async_api import async_playwright

HERE = Path(__file__).parent
HTML = HERE / "promo-hero.html"
OUT = HERE / "promo-hero.webm"

async def main():
    async with async_playwright() as p:
        browser = await p.chromium.launch()
        ctx = await browser.new_context(
            viewport={"width": 1200, "height": 630},
            device_scale_factor=2,
            record_video_dir=str(HERE),
            record_video_size={"width": 1200, "height": 630},
        )
        page = await ctx.new_page()
        await page.goto(f"file://{HTML.resolve()}")
        await page.wait_for_timeout(6000)  # 6 seconds of animation
        await ctx.close()  # saves the video
        await browser.close()

    # Rename the auto-generated video
    for f in HERE.glob("*.webm"):
        if f.name != "promo-hero.webm":
            f.rename(OUT)
            break

    print(f"Saved: {OUT}")
    print(f"Convert to mp4: ffmpeg -i {OUT} -c:v libx264 -pix_fmt yuv420p {HERE}/promo-hero.mp4")

asyncio.run(main())
